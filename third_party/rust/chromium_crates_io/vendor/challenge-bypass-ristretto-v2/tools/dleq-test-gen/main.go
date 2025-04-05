package main

import (
	"encoding/base64"
	"flag"
	"fmt"
	"hash"
	"io"
	"strings"

	"crypto/sha512"

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

func RandPoint(rng io.Reader) *ristretto.Point {
	var bytes [64]byte
	rng.Read(bytes[:])

	var buf [32]byte

	copy(buf[:], bytes[:32])
	var p1 ristretto.Point
	p1.SetElligator(&buf)

	copy(buf[:], bytes[32:])
	var p2 ristretto.Point
	p2.SetElligator(&buf)

	p1.Add(&p1, &p2)
	return &p1
}

func ScalarFromHash(h hash.Hash) *ristretto.Scalar {
	var s ristretto.Scalar
	var sBuf [64]byte
	sum := h.Sum([]byte{})
	copy(sBuf[:], sum[:])
	s.SetReduced(&sBuf)
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
var num = flag.Int("n", 5, "num vectors")

func new_dleq_proof(P *ristretto.Point, Q *ristretto.Point, k *ristretto.Scalar, Y *ristretto.Point) (*ristretto.Scalar, *ristretto.Scalar) {
	seed := [32]byte{}
	reader, err := rand.New(seed[:])
	if err != nil {
		log.Fatalln(err)
	}

	var A ristretto.Point
	var B ristretto.Point

	t := RandScalar(reader)
	A.ScalarMultBase(t)
	B.ScalarMult(P, t)

	var X ristretto.Point
	X.SetBase()
	h := sha512.New()
	h.Write(X.Bytes())
	h.Write(Y.Bytes())
	h.Write(P.Bytes())
	h.Write(Q.Bytes())
	h.Write(A.Bytes())
	h.Write(B.Bytes())

	c := ScalarFromHash(h)

	var s ristretto.Scalar
	var tmp ristretto.Scalar
	s.Sub(t, tmp.Mul(c, k))

	return c, &s
}

func calculate_batch_dleq_composites(P []*ristretto.Point, Q []*ristretto.Point, k *ristretto.Scalar, Y *ristretto.Point) (*ristretto.Point, *ristretto.Point) {
	var X ristretto.Point
	X.SetBase()
	h := sha512.New()
	h.Write(X.Bytes())
	h.Write(Y.Bytes())

	for i := 0; i < len(P); i++ {
		h.Write(P[i].Bytes())
		h.Write(Q[i].Bytes())
	}

	var seed [32]byte
	result := h.Sum([]byte{})
	copy(seed[:], result[:])

	reader, err := rand.New(seed[:])
	if err != nil {
		log.Fatalln(err)
	}

	var c_m []*ristretto.Scalar
	var M ristretto.Point
	var Z ristretto.Point

	for i := 0; i < len(P); i++ {
		c_i := RandScalar(reader)
		c_m = append(c_m, c_i)
		var tmp ristretto.Point
		if i == 0 {
			M = *tmp.ScalarMult(P[i], c_i)
			Z = *tmp.ScalarMult(Q[i], c_i)
		} else {
			M.Add(&M, tmp.ScalarMult(P[i], c_i))
			Z.Add(&Z, tmp.ScalarMult(Q[i], c_i))
		}
	}

	return &M, &Z
}

func main() {
	flag.Parse()

	if *verbose {
		log.SetLevel(log.DebugLevel)
	}

	seed := [32]byte{}
	reader, err := rand.New(seed[:])
	if err != nil {
		log.Fatalln(err)
	}

	fmt.Println("works vectors:")

	for i := 0; i < *num; i++ {
		vector := Vector{}

		k := RandScalar(reader)
		vector.Add("k", base64.StdEncoding.EncodeToString(k.Bytes()))
		var Y ristretto.Point
		Y.ScalarMultBase(k)
		vector.Add("Y", base64.StdEncoding.EncodeToString(Y.Bytes()))

		P := RandPoint(reader)
		vector.Add("P", base64.StdEncoding.EncodeToString(P.Bytes()))
		var Q ristretto.Point
		Q.ScalarMult(P, k)
		vector.Add("Q", base64.StdEncoding.EncodeToString(Q.Bytes()))

		c, s := new_dleq_proof(P, &Q, k, &Y)

		vector.Add("dleq", base64.StdEncoding.EncodeToString(append(c.Bytes(), s.Bytes()...)))

		fmt.Println(vector)
	}

	fmt.Println("batch_works vectors:")

	for i := 0; i < *num; i++ {
		vector := Vector{}

		k := RandScalar(reader)
		vector.Add("k", base64.StdEncoding.EncodeToString(k.Bytes()))
		var Y ristretto.Point
		Y.ScalarMultBase(k)
		vector.Add("Y", base64.StdEncoding.EncodeToString(Y.Bytes()))

		var P []*ristretto.Point
		var P_str []string
		var Q []*ristretto.Point
		var Q_str []string
		for j := 0; j < 5; j++ {
			P_i := RandPoint(reader)
			P_str = append(P_str, base64.StdEncoding.EncodeToString(P_i.Bytes()))
			var Q_i ristretto.Point
			Q_i.ScalarMult(P_i, k)
			Q_str = append(Q_str, base64.StdEncoding.EncodeToString(Q_i.Bytes()))
			P = append(P, P_i)
			Q = append(Q, &Q_i)
		}
		vector.Add("P", strings.Join(P_str, ","))
		vector.Add("Q", strings.Join(Q_str, ","))

		M, Z := calculate_batch_dleq_composites(P, Q, k, &Y)
		vector.Add("M", base64.StdEncoding.EncodeToString(M.Bytes()))
		vector.Add("Z", base64.StdEncoding.EncodeToString(Z.Bytes()))

		c, s := new_dleq_proof(M, Z, k, &Y)

		vector.Add("dleq", base64.StdEncoding.EncodeToString(append(c.Bytes(), s.Bytes()...)))

		fmt.Println(vector)
	}
}
