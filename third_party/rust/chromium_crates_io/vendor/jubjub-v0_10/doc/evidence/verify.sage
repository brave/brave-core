import os
import sys
from errno import ENOENT, EEXIST
from sortedcontainers import SortedSet


def readfile(fn):
  fd = open(fn,'r')
  r = fd.read()
  fd.close()
  return r

def writefile(fn,s):
  fd = open(fn,'w')
  fd.write(s)
  fd.close()

def expand2(n):
  s = ""
  
  while n != 0:
    j = 16
    while 2**j < abs(n): j += 1
    if 2**j - abs(n) > abs(n) - 2**(j-1): j -= 1
  
    if abs(abs(n) - 2**j) > 2**(j - 10):
      if n > 0:
        if s != "": s += " + "
        s += str(n)
      else:
        s += " - " + str(-n)
      n = 0
    elif n > 0:
      if s != "": s += " + "
      s += "2^" + str(j)
      n -= 2**j
    else:
      s += " - 2^" + str(j)
      n += 2**j
  
  return s

def requirement(fn,istrue):
  writefile(fn,str(istrue) + '\n')
  return istrue

def verify():
  try:
    os.mkdir('proof')
  except OSError as e:
    if e.errno != EEXIST: raise

  try:
    s = set(map(Integer, readfile('primes').split()))
  except IOError, e:
    if e.errno != ENOENT: raise
    s = set()

  needtofactor = SortedSet()
  V = SortedSet() # distinct verified primes
  verify_primes(V, s, needtofactor)
  verify_pass(V, needtofactor)

  old = V
  needtofactor.update(V)
  while len(needtofactor) > len(old):
    k = len(needtofactor) - len(old)
    sys.stdout.write('Factoring %d integer%s' % (k, '' if k == 1 else 's'))
    sys.stdout.flush()
    for x in needtofactor:
      if x not in old:
        for (y, z) in factor(x):
          s.add(y)
        sys.stdout.write('.')
        sys.stdout.flush()

    print('')

    old = needtofactor.copy()
    verify_primes(V, s, needtofactor)

  writefile('primes', '\n'.join(map(str, s)) + '\n')
  writefile('verify-primes', '<html><body>\n' +
                             ''.join(('2\n' if v == 2 else
                                      '<a href=proof/%s.html>%s</a>\n' % (v,v)) for v in V) +
                             '</body></html>\n')

  verify_pass(V, needtofactor)


def verify_primes(V, s, needtofactor):
  for n in sorted(s):
    if not n.is_prime() or n in V: continue
    needtofactor.add(n-1)
    if n == 2:
      V.add(n)
      continue
    for trybase in primes(2,10000):
      base = Integers(n)(trybase)
      if base^(n-1) != 1: continue
      proof = 'Primality proof for n = %s:\n' % n
      proof += '<p>Take b = %s.\n' % base
      proof += '<p>b^(n-1) mod n = 1.\n'
      f = factor(1)
      for v in reversed(V):
        if f.prod()^2 <= n:
          if n % v == 1:
            u = base^((n-1)/v)-1
            if u.is_unit():
              if v == 2:
                proof += '<p>2 is prime.\n'
              else:
                proof += '<p><a href=%s.html>%s is prime.</a>\n' % (v,v)
              proof += '<br>b^((n-1)/%s)-1 mod n = %s, which is a unit, inverse %s.\n' % (v,u,1/u)
              f *= factor(v)^(n-1).valuation(v)
      if f.prod()^2 <= n: continue
      if n % f.prod() != 1: continue
      proof += '<p>(%s) divides n-1.\n' % f
      proof += '<p>(%s)^2 > n.\n' % f
      proof += "<p>n is prime by Pocklington's theorem.\n"
      proof += '\n'
      writefile('proof/%s.html' % n,proof)
      V.add(n)
      break


def verify_pass(V, needtofactor):
  p = Integer(readfile('p'))
  k = GF(p)
  kz.<z> = k[]
  l = Integer(readfile('l'))
  x0 = Integer(readfile('x0'))
  y0 = Integer(readfile('y0'))
  x1 = Integer(readfile('x1'))
  y1 = Integer(readfile('y1'))
  shape = readfile('shape').strip()
  rigid = readfile('rigid').strip()

  safefield = True
  safeeq = True
  safebase = True
  saferho = True
  safetransfer = True
  safedisc = True
  saferigid = True
  safeladder = True
  safetwist = True
  safecomplete = True
  safeind = True

  pstatus = 'Unverified'
  if not p.is_prime(): pstatus = 'False'
  needtofactor.add(p)
  if p in V: pstatus = 'True'
  if pstatus != 'True': safefield = False
  writefile('verify-pisprime',pstatus + '\n')

  pstatus = 'Unverified'
  if not l.is_prime(): pstatus = 'False'
  needtofactor.add(l)
  if l in V: pstatus = 'True'
  if pstatus != 'True': safebase = False
  writefile('verify-lisprime',pstatus + '\n')

  writefile('expand2-p','= %s\n' % expand2(p))
  writefile('expand2-l','<br>= %s\n' % expand2(l))
  
  writefile('hex-p',hex(p) + '\n')
  writefile('hex-l',hex(l) + '\n')
  writefile('hex-x0',hex(x0) + '\n')
  writefile('hex-x1',hex(x1) + '\n')
  writefile('hex-y0',hex(y0) + '\n')
  writefile('hex-y1',hex(y1) + '\n')

  gcdlpis1 = gcd(l,p) == 1
  safetransfer &= requirement('verify-gcdlp1',gcdlpis1)

  writefile('verify-movsafe','Unverified\n')
  writefile('verify-embeddingdegree','Unverified\n')
  if gcdlpis1 and l.is_prime():
    u = Integers(l)(p)
    d = l-1
    needtofactor.add(d)
    for v in V:
      while d % v == 0: d /= v
    if d == 1:
      d = l-1
      for v in V:
        while d % v == 0:
          if u^(d/v) != 1: break
          d /= v
      safetransfer &= requirement('verify-movsafe',(l-1)/d <= 100)
      writefile('verify-embeddingdegree','<font size=1>%s</font><br>= (l-1)/%s\n' % (d,(l-1)/d))

  t = p+1-l*round((p+1)/l)
  if l^2 > 16*p:
    writefile('verify-trace','%s\n' % t)
    f = factor(1)
    d = (p+1-t)/l
    needtofactor.add(d)
    for v in V:
      while d % v == 0:
        d //= v
	f *= factor(v)
    writefile('verify-cofactor','%s\n' % f)
  else:
    writefile('verify-trace','Unverified\n')
    writefile('verify-cofactor','Unverified\n')

  D = t^2-4*p
  needtofactor.add(D)
  for v in V:
    while D % v^2 == 0: D /= v^2
  if prod([v for v in V if D % v == 0]) != -D:
    writefile('verify-disc','Unverified\n')
    writefile('verify-discisbig','Unverified\n')
    safedisc = False
  else:
    f = -prod([factor(v) for v in V if D % v == 0])
    if D % 4 != 1:
      D *= 4
      f = factor(4) * f
    Dbits = (log(-D)/log(2)).numerical_approx()
    writefile('verify-disc','<font size=1>%s</font><br>= <font size=1>%s</font><br>&#x2248; -2^%.1f\n' % (D,f,Dbits))
    safedisc &= requirement('verify-discisbig',D < -2^100)

  pi4 = 0.78539816339744830961566084581987572105
  rho = log(pi4*l)/log(4)
  writefile('verify-rho','%.1f\n' % rho)
  saferho &= requirement('verify-rhoabove100',rho.numerical_approx() >= 100)

  twistl = 'Unverified'
  d = p+1+t
  needtofactor.add(d)
  for v in V:
    while d % v == 0: d /= v
  if d == 1:
    d = p+1+t
    for v in V:
      if d % v == 0:
        if twistl == 'Unverified' or v > twistl: twistl = v

  writefile('verify-twistl','%s\n' % twistl)
  writefile('verify-twistembeddingdegree','Unverified\n')
  writefile('verify-twistmovsafe','Unverified\n')
  if twistl == 'Unverified':
    writefile('hex-twistl','Unverified\n')
    writefile('expand2-twistl','Unverified\n')
    writefile('verify-twistcofactor','Unverified\n')
    writefile('verify-gcdtwistlp1','Unverified\n')
    writefile('verify-twistrho','Unverified\n')
    safetwist = False
  else:
    writefile('hex-twistl',hex(twistl) + '\n')
    writefile('expand2-twistl','<br>= %s\n' % expand2(twistl))
    f = factor(1)
    d = (p+1+t)/twistl
    needtofactor.add(d)
    for v in V:
      while d % v == 0:
        d //= v
	f *= factor(v)
    writefile('verify-twistcofactor','%s\n' % f)
    gcdtwistlpis1 = gcd(twistl,p) == 1
    safetwist &= requirement('verify-gcdtwistlp1',gcdtwistlpis1)

    movsafe = 'Unverified'
    embeddingdegree = 'Unverified'
    if gcdtwistlpis1 and twistl.is_prime():
      u = Integers(twistl)(p)
      d = twistl-1
      needtofactor.add(d)
      for v in V:
        while d % v == 0: d /= v
      if d == 1:
        d = twistl-1
        for v in V:
          while d % v == 0:
            if u^(d/v) != 1: break
            d /= v
        safetwist &= requirement('verify-twistmovsafe',(twistl-1)/d <= 100)
        writefile('verify-twistembeddingdegree',"<font size=1>%s</font><br>= (l'-1)/%s\n" % (d,(twistl-1)/d))

    rho = log(pi4*twistl)/log(4)
    writefile('verify-twistrho','%.1f\n' % rho)
    safetwist &= requirement('verify-twistrhoabove100',rho.numerical_approx() >= 100)

    precomp = 0
    joint = l
    needtofactor.add(p+1-t)
    needtofactor.add(p+1+t)
    for v in V:
      d1 = p+1-t
      d2 = p+1+t
      while d1 % v == 0 or d2 % v == 0:
        if d1 % v == 0: d1 //= v
        if d2 % v == 0: d2 //= v
        # best case for attack: cyclic; each power is usable
	# also assume that kangaroo is as efficient as rho
        if v + sqrt(pi4*joint/v) < sqrt(pi4*joint):
	  precomp += v
	  joint /= v
        
    rho = log(precomp + sqrt(pi4 * joint))/log(2)
    writefile('verify-jointrho','%.1f\n' % rho)
    safetwist &= requirement('verify-jointrhoabove100',rho.numerical_approx() >= 100)


  x0 = k(x0)
  y0 = k(y0)
  x1 = k(x1)
  y1 = k(y1)

  if shape in ('edwards', 'tedwards'):
    d = Integer(readfile('d'))
    a = 1
    if shape == 'tedwards':
      a = Integer(readfile('a'))

    writefile('verify-shape','Twisted Edwards\n')
    writefile('verify-equation','%sx^2+y^2 = 1%+dx^2y^2\n' % (a, d))
    if a == 1:
      writefile('verify-shape','Edwards\n')
      writefile('verify-equation','x^2+y^2 = 1%+dx^2y^2\n' % d)

    a = k(a)
    d = k(d)
    elliptic = a*d*(a-d)
    level0 = a*x0^2+y0^2-1-d*x0^2*y0^2
    level1 = a*x1^2+y1^2-1-d*x1^2*y1^2

  if shape == 'montgomery':
    writefile('verify-shape','Montgomery\n')
    A = Integer(readfile('A'))
    B = Integer(readfile('B'))
    equation = '%sy^2 = x^3<wbr>%+dx^2+x' % (B,A)
    if B == 1:
      equation = 'y^2 = x^3<wbr>%+dx^2+x' % A
    writefile('verify-equation',equation + '\n')

    A = k(A)
    B = k(B)
    elliptic = B*(A^2-4)
    level0 = B*y0^2-x0^3-A*x0^2-x0
    level1 = B*y1^2-x1^3-A*x1^2-x1

  if shape == 'shortw':
    writefile('verify-shape','short Weierstrass\n')
    a = Integer(readfile('a'))
    b = Integer(readfile('b'))
    writefile('verify-equation','y^2 = x^3<wbr>%+dx<wbr>%+d\n' % (a,b))

    a = k(a)
    b = k(b)
    elliptic = 4*a^3+27*b^2
    level0 = y0^2-x0^3-a*x0-b
    level1 = y1^2-x1^3-a*x1-b

  writefile('verify-elliptic',str(elliptic) + '\n')
  safeeq &= requirement('verify-iselliptic',elliptic != 0)
  safebase &= requirement('verify-isoncurve0',level0 == 0)
  safebase &= requirement('verify-isoncurve1',level1 == 0)

  if shape in ('edwards', 'tedwards'):
    A = 2*(a+d)/(a-d)
    B = 4/(a-d)
    x0,y0 = (1+y0)/(1-y0),((1+y0)/(1-y0))/x0
    x1,y1 = (1+y1)/(1-y1),((1+y1)/(1-y1))/x1
    shape = 'montgomery'

  if shape == 'montgomery':
    a = (3-A^2)/(3*B^2)
    b = (2*A^3-9*A)/(27*B^3)
    x0,y0 = (x0+A/3)/B,y0/B
    x1,y1 = (x1+A/3)/B,y1/B
    shape = 'shortw'

  try:
    E = EllipticCurve([a,b])
    numorder2 = 0
    numorder4 = 0
    for P in E(0).division_points(4):
      if P != 0 and 2*P == 0:
        numorder2 += 1
      if 2*P != 0 and 4*P == 0:
        numorder4 += 1
    writefile('verify-numorder2',str(numorder2) + '\n')
    writefile('verify-numorder4',str(numorder4) + '\n')
    completesingle = False
    completemulti = False
    if numorder4 == 2 and numorder2 == 1:
      # complete edwards form, and montgomery with unique point of order 2
      completesingle = True
      completemulti = True
    # should extend this to allow complete twisted hessian
    safecomplete &= requirement('verify-completesingle',completesingle)
    safecomplete &= requirement('verify-completemulti',completemulti)
    safecomplete &= requirement('verify-ltimesbase1is0',l * E([x1,y1]) == 0)
    writefile('verify-ltimesbase1',str(l * E([x1,y1])) + '\n')
    writefile('verify-cofactorbase01',str(((p+1-t)//l) * E([x0,y0]) == E([x1,y1])) + '\n')
  except:
    writefile('verify-numorder2','Unverified\n')
    writefile('verify-numorder4','Unverified\n')
    writefile('verify-ltimesbase1','Unverified\n')
    writefile('verify-cofactorbase01','Unverified\n')
    safecomplete = False

  montladder = False
  for r,e in (z^3+a*z+b).roots():
    if (3*r^2+a).is_square():
      montladder = True
  safeladder &= requirement('verify-montladder',montladder)

  indistinguishability = False
  elligator2 = False
  if (p+1-t) % 2 == 0:
    if b != 0:
      indistinguishability = True
      elligator2 = True
  safeind &= requirement('verify-indistinguishability',indistinguishability)
  writefile('verify-ind-notes','Elligator 2: %s.\n' % ['No','Yes'][elligator2])

  saferigid &= (rigid == 'fully rigid' or rigid == 'somewhat rigid')

  safecurve = True
  safecurve &= requirement('verify-safefield',safefield)
  safecurve &= requirement('verify-safeeq',safeeq)
  safecurve &= requirement('verify-safebase',safebase)
  safecurve &= requirement('verify-saferho',saferho)
  safecurve &= requirement('verify-safetransfer',safetransfer)
  safecurve &= requirement('verify-safedisc',safedisc)
  safecurve &= requirement('verify-saferigid',saferigid)
  safecurve &= requirement('verify-safeladder',safeladder)
  safecurve &= requirement('verify-safetwist',safetwist)
  safecurve &= requirement('verify-safecomplete',safecomplete)
  safecurve &= requirement('verify-safeind',safeind)
  requirement('verify-safecurve',safecurve)

originaldir = os.open('.',os.O_RDONLY)
for i in range(1,len(sys.argv)):
  os.fchdir(originaldir)
  os.chdir(sys.argv[i])
  verify()

