package main

import (
	"encoding/base64"
	"flag"
	"fmt"
	"io"
	"strings"

	ristretto "github.com/bwesterb/go-ristretto"
	log "github.com/sirupsen/logrus"
	rand "github.com/tmthrgd/go-rand"
)

func RandScalar(rng io.Reader) *ristretto.Scalar {
	var s ristretto.Scalar
	var buf [64]byte
	rng.Read(buf[:])
	s.SetReduced(&buf)
	return &s
}

type Vector struct {
	elements []string
}

func (v *Vector) Add(key string, value string) {
	log.Debug(key + ": " + value)
	v.elements = append(v.elements, value)
}

func (v Vector) String() string {
	return "(\"" + strings.Join(v.elements, "\", \"") + "\"),"
}

var verbose = flag.Bool("v", false, "verbose output")
var num = flag.Int("n", 10, "num vectors")

func main() {
	prng_seed := [32]byte{}
	reader, err := rand.New(prng_seed[:])
	if err != nil {
		log.Fatalln(err)
	}

	flag.Parse()

	if *verbose {
		log.SetLevel(log.DebugLevel)
	}

	for i := 0; i < *num; i++ {
		var Y ristretto.Point

		vector := Vector{}
		k := RandScalar(reader) // generate a new secret key
		Y.ScalarMultBase(k)

		vector.Add("k", base64.StdEncoding.EncodeToString(k.Bytes()))

		Y.ScalarMultBase(k) // compute public key
		vector.Add("Y", base64.StdEncoding.EncodeToString(Y.Bytes()))

		token_seed := make([]byte, 64)
		_, err := reader.Read(token_seed)
		if err != nil {
			log.Fatal(err)
		}

		vector.Add("seed", base64.StdEncoding.EncodeToString(token_seed))

		var T ristretto.Point
		T.DeriveDalek(token_seed)

		var P ristretto.Point

		r := RandScalar(reader)

		vector.Add("r", base64.StdEncoding.EncodeToString(r.Bytes()))

		P.ScalarMult(&T, r)

		vector.Add("P", base64.StdEncoding.EncodeToString(P.Bytes()))

		var Q ristretto.Point
		Q.ScalarMult(&P, k)

		vector.Add("Q", base64.StdEncoding.EncodeToString(Q.Bytes()))

		var r_inv ristretto.Scalar
		r_inv.Inverse(r)

		var W ristretto.Point
		W.ScalarMult(&Q, &r_inv)

		vector.Add("W", base64.StdEncoding.EncodeToString(W.Bytes()))

		fmt.Println(vector)
	}
}
