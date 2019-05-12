/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

#ifndef BN_H_
#define BN_H_

#include <stdint.h>
#include <limits.h>

#ifdef LTM_NO_FILE
#  warning LTM_NO_FILE has been deprecated, use MP_NO_FILE.
#  define MP_NO_FILE
#endif

#ifndef MP_NO_FILE
#  include <stdio.h>
#endif

#include "tommath_class.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MS Visual C++ doesn't have a 128bit type for words, so fall back to 32bit MPI's (where words are 64bit) */
#if defined(_MSC_VER) || defined(__LLP64__) || defined(__e2k__) || defined(__LCC__)
#   define MP_32BIT
#endif

/* detect 64-bit mode if possible */
#if defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64) || \
    defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__) || \
    defined(__s390x__) || defined(__arch64__) || defined(__aarch64__) || \
    defined(__sparcv9) || defined(__sparc_v9__) || defined(__sparc64__) || \
    defined(__ia64) || defined(__ia64__) || defined(__itanium__) || defined(_M_IA64) || \
    defined(__LP64__) || defined(_LP64) || defined(__64BIT__)
#   if !(defined(MP_32BIT) || defined(MP_16BIT) || defined(MP_8BIT))
#      if defined(__GNUC__)
/* we support 128bit integers only via: __attribute__((mode(TI))) */
#         define MP_64BIT
#      else
/* otherwise we fall back to MP_32BIT even on 64bit platforms */
#         define MP_32BIT
#      endif
#   endif
#endif

/* some default configurations.
 *
 * A "mp_digit" must be able to hold MP_DIGIT_BIT + 1 bits
 * A "mp_word" must be able to hold 2*MP_DIGIT_BIT + 1 bits
 *
 * At the very least a mp_digit must be able to hold 7 bits
 * [any size beyond that is ok provided it doesn't overflow the data type]
 */
#ifdef MP_8BIT
typedef uint8_t              mp_digit;
typedef uint16_t             mp_word;
#   define MP_SIZEOF_MP_DIGIT 1
#   ifdef MP_DIGIT_BIT
#      error You must not define MP_DIGIT_BIT when using MP_8BIT
#   endif
#elif defined(MP_16BIT)
typedef uint16_t             mp_digit;
typedef uint32_t             mp_word;
#   define MP_SIZEOF_MP_DIGIT 2
#   ifdef MP_DIGIT_BIT
#      error You must not define MP_DIGIT_BIT when using MP_16BIT
#   endif
#elif defined(MP_64BIT)
/* for GCC only on supported platforms */
typedef uint64_t mp_digit;
typedef unsigned long        mp_word __attribute__((mode(TI)));
#   define MP_DIGIT_BIT 60
#else
/* this is the default case, 28-bit digits */

/* this is to make porting into LibTomCrypt easier :-) */
typedef uint32_t             mp_digit;
typedef uint64_t             mp_word;

#   ifdef MP_31BIT
/* this is an extension that uses 31-bit digits */
#      define MP_DIGIT_BIT 31
#   else
/* default case is 28-bit digits, defines MP_28BIT as a handy macro to test */
#      define MP_DIGIT_BIT 28
#      define MP_28BIT
#   endif
#endif

/* otherwise the bits per digit is calculated automatically from the size of a mp_digit */
#ifndef MP_DIGIT_BIT
#   define MP_DIGIT_BIT (((CHAR_BIT * MP_SIZEOF_MP_DIGIT) - 1))  /* bits per digit */
#endif

#define MP_MASK          ((((mp_digit)1)<<((mp_digit)MP_DIGIT_BIT))-((mp_digit)1))
#define MP_DIGIT_MAX     MP_MASK

/* equalities */
#define MP_LT        -1   /* less than */
#define MP_EQ         0   /* equal to */
#define MP_GT         1   /* greater than */

#define MP_ZPOS       0   /* positive integer */
#define MP_NEG        1   /* negative */

#define MP_OKAY       0   /* ok result */
#define MP_ERR        -1  /* unknown error */
#define MP_MEM        -2  /* out of mem */
#define MP_VAL        -3  /* invalid input */
#define MP_RANGE      MP_VAL
#define MP_ITER       -4  /* Max. iterations reached */

#define MP_YES        1   /* yes response */
#define MP_NO         0   /* no response */

/* Primality generation flags */
#define MP_PRIME_BBS      0x0001 /* BBS style prime */
#define MP_PRIME_SAFE     0x0002 /* Safe prime (p-1)/2 == prime */
#define MP_PRIME_2MSB_ON  0x0008 /* force 2nd MSB to 1 */

#define LTM_PRIME_BBS      (MP_DEPRECATED_PRAGMA("LTM_PRIME_BBS has been deprecated, use MP_PRIME_BBS") MP_PRIME_BBS)
#define LTM_PRIME_SAFE     (MP_DEPRECATED_PRAGMA("LTM_PRIME_SAFE has been deprecated, use MP_PRIME_SAFE") MP_PRIME_SAFE)
#define LTM_PRIME_2MSB_ON  (MP_DEPRECATED_PRAGMA("LTM_PRIME_2MSB_ON has been deprecated, use MP_PRIME_2MSB_ON") MP_PRIME_2MSB_ON)

typedef int           mp_err;

/* tunable cutoffs */

#ifndef MP_FIXED_CUTOFFS
extern int
KARATSUBA_MUL_CUTOFF,
KARATSUBA_SQR_CUTOFF,
TOOM_MUL_CUTOFF,
TOOM_SQR_CUTOFF;
#endif

/* define this to use lower memory usage routines (exptmods mostly) */
/* #define MP_LOW_MEM */

/* default precision */
#ifndef MP_PREC
#   ifndef MP_LOW_MEM
#      define MP_PREC 32        /* default digits of precision */
#   elif defined(MP_8BIT)
#      define MP_PREC 16        /* default digits of precision */
#   else
#      define MP_PREC 8         /* default digits of precision */
#   endif
#endif

/* size of comba arrays, should be at least 2 * 2**(BITS_PER_WORD - BITS_PER_DIGIT*2) */
#define PRIVATE_MP_WARRAY (1u << (((CHAR_BIT * sizeof(mp_word)) - (2 * MP_DIGIT_BIT)) + 1))
#define MP_WARRAY (MP_DEPRECATED_PRAGMA("MP_WARRAY is an internal macro") PRIVATE_MP_WARRAY)

#if defined(__GNUC__) && __GNUC__ >= 4
#   define MP_NULL_TERMINATED __attribute__((sentinel))
#else
#   define MP_NULL_TERMINATED
#endif

/*
 * MP_WUR - warn unused result
 * ---------------------------
 *
 * The result of functions annotated with MP_WUR must be
 * checked and cannot be ignored.
 *
 * Most functions in libtommath return an error code.
 * This error code must be checked in order to prevent crashes or invalid
 * results.
 *
 * If you still want to avoid the error checks for quick and dirty programs
 * without robustness guarantees, you can `#define MP_WUR` before including
 * tommath.h, disabling the warnings.
 */
#ifndef MP_WUR
#  if defined(__GNUC__) && __GNUC__ >= 4
#     define MP_WUR __attribute__((warn_unused_result))
#  else
#     define MP_WUR
#  endif
#endif

#if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 301)
#  define MP_DEPRECATED(x) __attribute__((deprecated("replaced by " #x)))
#  define PRIVATE_MP_DEPRECATED_PRAGMA(s) _Pragma(#s)
#  define MP_DEPRECATED_PRAGMA(s) PRIVATE_MP_DEPRECATED_PRAGMA(GCC warning s)
#elif defined(_MSC_VER) && _MSC_VER >= 1500
#  define MP_DEPRECATED(x) __declspec(deprecated("replaced by " #x))
#  define MP_DEPRECATED_PRAGMA(s) __pragma(message(s))
#else
#  define MP_DEPRECATED
#  define MP_DEPRECATED_PRAGMA(s)
#endif

#define DIGIT_BIT   (MP_DEPRECATED_PRAGMA("DIGIT_BIT macro is deprecated, MP_DIGIT_BIT instead") MP_DIGIT_BIT)
#define USED(m)     (MP_DEPRECATED_PRAGMA("USED macro is deprecated, use z->used instead") (m)->used)
#define DIGIT(m, k) (MP_DEPRECATED_PRAGMA("DIGIT macro is deprecated, use z->dp instead") (m)->dp[(k)])
#define SIGN(m)     (MP_DEPRECATED_PRAGMA("SIGN macro is deprecated, use z->sign instead") (m)->sign)

/* the infamous mp_int structure */
typedef struct  {
   int used, alloc, sign;
   mp_digit *dp;
} mp_int;

/* callback for mp_prime_random, should fill dst with random bytes and return how many read [upto len] */
typedef int private_mp_prime_callback(unsigned char *dst, int len, void *dat);
typedef private_mp_prime_callback ltm_prime_callback MP_DEPRECATED(mp_rand_source);

/* error code to char* string */
const char *mp_error_to_string(int code);

/* ---> init and deinit bignum functions <--- */
/* init a bignum */
MP_WUR int mp_init(mp_int *a);

/* free a bignum */
void mp_clear(mp_int *a);

/* init a null terminated series of arguments */
MP_WUR int mp_init_multi(mp_int *mp, ...) MP_NULL_TERMINATED;

/* clear a null terminated series of arguments */
void mp_clear_multi(mp_int *mp, ...) MP_NULL_TERMINATED;

/* exchange two ints */
void mp_exch(mp_int *a, mp_int *b);

/* shrink ram required for a bignum */
MP_WUR int mp_shrink(mp_int *a);

/* grow an int to a given size */
MP_WUR int mp_grow(mp_int *a, int size);

/* init to a given number of digits */
MP_WUR int mp_init_size(mp_int *a, int size);

/* ---> Basic Manipulations <--- */
#define mp_iszero(a) (((a)->used == 0) ? MP_YES : MP_NO)
MP_WUR int mp_iseven(const mp_int *a);
MP_WUR int mp_isodd(const mp_int *a);
#define mp_isneg(a)  (((a)->sign != MP_ZPOS) ? MP_YES : MP_NO)

/* set to zero */
void mp_zero(mp_int *a);

/* set to a digit */
void mp_set(mp_int *a, mp_digit b);

/* set a double */
MP_WUR int mp_set_double(mp_int *a, double b);

/* set a 32-bit const */
/* TODO void - never fails */ int mp_set_int(mp_int *a, unsigned long b);

/* set a platform dependent unsigned long value */
/* TODO void - never fails */ int mp_set_long(mp_int *a, unsigned long b);

/* set a platform dependent unsigned long long value */
/* TODO void - never fails */ int mp_set_long_long(mp_int *a, unsigned long long b);

/* get a double */
MP_WUR double mp_get_double(const mp_int *a);

/* get a 32-bit value */
MP_WUR unsigned long mp_get_int(const mp_int *a);

/* get a platform dependent unsigned long value */
MP_WUR unsigned long mp_get_long(const mp_int *a);

/* get a platform dependent unsigned long long value */
MP_WUR unsigned long long mp_get_long_long(const mp_int *a);

/* initialize and set a digit */
MP_WUR int mp_init_set(mp_int *a, mp_digit b);

/* initialize and set 32-bit value */
MP_WUR int mp_init_set_int(mp_int *a, unsigned long b);

/* copy, b = a */
MP_WUR int mp_copy(const mp_int *a, mp_int *b);

/* inits and copies, a = b */
MP_WUR int mp_init_copy(mp_int *a, const mp_int *b);

/* trim unused digits */
void mp_clamp(mp_int *a);

/* import binary data */
MP_WUR int mp_import(mp_int *rop, size_t count, int order, size_t size, int endian, size_t nails, const void *op);

/* export binary data */
MP_WUR int mp_export(void *rop, size_t *countp, int order, size_t size, int endian, size_t nails, const mp_int *op);

/* ---> digit manipulation <--- */

/* right shift by "b" digits */
void mp_rshd(mp_int *a, int b);

/* left shift by "b" digits */
MP_WUR int mp_lshd(mp_int *a, int b);

/* c = a / 2**b, implemented as c = a >> b */
MP_WUR int mp_div_2d(const mp_int *a, int b, mp_int *c, mp_int *d);

/* b = a/2 */
MP_WUR int mp_div_2(const mp_int *a, mp_int *b);

/* c = a * 2**b, implemented as c = a << b */
MP_WUR int mp_mul_2d(const mp_int *a, int b, mp_int *c);

/* b = a*2 */
MP_WUR int mp_mul_2(const mp_int *a, mp_int *b);

/* c = a mod 2**b */
MP_WUR int mp_mod_2d(const mp_int *a, int b, mp_int *c);

/* computes a = 2**b */
MP_WUR int mp_2expt(mp_int *a, int b);

/* Counts the number of lsbs which are zero before the first zero bit */
MP_WUR int mp_cnt_lsb(const mp_int *a);

/* I Love Earth! */

/* makes a pseudo-random mp_int of a given size */
MP_WUR int mp_rand(mp_int *a, int digits);
/* makes a pseudo-random small int of a given size */
MP_WUR MP_DEPRECATED(mp_rand) int mp_rand_digit(mp_digit *r);
/* use custom random data source instead of source provided the platform */
void mp_rand_source(int source(void *out, size_t size));

#ifdef MP_PRNG_ENABLE_LTM_RNG
#  warning MP_PRNG_ENABLE_LTM_RNG has been deprecated, use mp_rand_source instead.
/* A last resort to provide random data on systems without any of the other
 * implemented ways to gather entropy.
 * It is compatible with `rng_get_bytes()` from libtomcrypt so you could
 * provide that one and then set `ltm_rng = rng_get_bytes;` */
extern unsigned long (*ltm_rng)(unsigned char *out, unsigned long outlen, void (*callback)(void));
extern void (*ltm_rng_callback)(void);
#endif

/* ---> binary operations <--- */
/* c = a XOR b  */
MP_WUR int mp_xor(const mp_int *a, const mp_int *b, mp_int *c);

/* c = a OR b */
MP_WUR int mp_or(const mp_int *a, const mp_int *b, mp_int *c);

/* c = a AND b */
MP_WUR int mp_and(const mp_int *a, const mp_int *b, mp_int *c);

/* Checks the bit at position b and returns MP_YES
   if the bit is 1, MP_NO if it is 0 and MP_VAL
   in case of error */
MP_WUR int mp_get_bit(const mp_int *a, int b);

/* c = a XOR b (two complement) */
MP_WUR int mp_tc_xor(const mp_int *a, const mp_int *b, mp_int *c);

/* c = a OR b (two complement) */
MP_WUR int mp_tc_or(const mp_int *a, const mp_int *b, mp_int *c);

/* c = a AND b (two complement) */
MP_WUR int mp_tc_and(const mp_int *a, const mp_int *b, mp_int *c);

/* right shift (two complement) */
MP_WUR int mp_tc_div_2d(const mp_int *a, int b, mp_int *c);

/* ---> Basic arithmetic <--- */

/* b = ~a */
MP_WUR int mp_complement(const mp_int *a, mp_int *b);

/* b = -a */
MP_WUR int mp_neg(const mp_int *a, mp_int *b);

/* b = |a| */
MP_WUR int mp_abs(const mp_int *a, mp_int *b);

/* compare a to b */
MP_WUR int mp_cmp(const mp_int *a, const mp_int *b);

/* compare |a| to |b| */
MP_WUR int mp_cmp_mag(const mp_int *a, const mp_int *b);

/* c = a + b */
MP_WUR int mp_add(const mp_int *a, const mp_int *b, mp_int *c);

/* c = a - b */
MP_WUR int mp_sub(const mp_int *a, const mp_int *b, mp_int *c);

/* c = a * b */
MP_WUR int mp_mul(const mp_int *a, const mp_int *b, mp_int *c);

/* b = a*a  */
MP_WUR int mp_sqr(const mp_int *a, mp_int *b);

/* a/b => cb + d == a */
MP_WUR int mp_div(const mp_int *a, const mp_int *b, mp_int *c, mp_int *d);

/* c = a mod b, 0 <= c < b  */
MP_WUR int mp_mod(const mp_int *a, const mp_int *b, mp_int *c);

/* ---> single digit functions <--- */

/* compare against a single digit */
MP_WUR int mp_cmp_d(const mp_int *a, mp_digit b);

/* c = a + b */
MP_WUR int mp_add_d(const mp_int *a, mp_digit b, mp_int *c);

/* Increment "a" by one like "a++". Changes input! */
MP_WUR int mp_incr(mp_int *a);

/* c = a - b */
MP_WUR int mp_sub_d(const mp_int *a, mp_digit b, mp_int *c);

/* Decrement "a" by one like "a--". Changes input! */
MP_WUR int mp_decr(mp_int *a);

/* c = a * b */
MP_WUR int mp_mul_d(const mp_int *a, mp_digit b, mp_int *c);

/* a/b => cb + d == a */
MP_WUR int mp_div_d(const mp_int *a, mp_digit b, mp_int *c, mp_digit *d);

/* a/3 => 3c + d == a */
MP_WUR int mp_div_3(const mp_int *a, mp_int *c, mp_digit *d);

/* c = a**b */
MP_WUR int mp_expt_d(const mp_int *a, mp_digit b, mp_int *c);
MP_WUR int mp_expt_d_ex(const mp_int *a, mp_digit b, mp_int *c, int fast);

/* c = a mod b, 0 <= c < b  */
MP_WUR int mp_mod_d(const mp_int *a, mp_digit b, mp_digit *c);

/* ---> number theory <--- */

/* d = a + b (mod c) */
MP_WUR int mp_addmod(const mp_int *a, const mp_int *b, const mp_int *c, mp_int *d);

/* d = a - b (mod c) */
MP_WUR int mp_submod(const mp_int *a, const mp_int *b, const mp_int *c, mp_int *d);

/* d = a * b (mod c) */
MP_WUR int mp_mulmod(const mp_int *a, const mp_int *b, const mp_int *c, mp_int *d);

/* c = a * a (mod b) */
MP_WUR int mp_sqrmod(const mp_int *a, const mp_int *b, mp_int *c);

/* c = 1/a (mod b) */
MP_WUR int mp_invmod(const mp_int *a, const mp_int *b, mp_int *c);

/* c = (a, b) */
MP_WUR int mp_gcd(const mp_int *a, const mp_int *b, mp_int *c);

/* produces value such that U1*a + U2*b = U3 */
MP_WUR int mp_exteuclid(const mp_int *a, const mp_int *b, mp_int *U1, mp_int *U2, mp_int *U3);

/* c = [a, b] or (a*b)/(a, b) */
MP_WUR int mp_lcm(const mp_int *a, const mp_int *b, mp_int *c);

/* finds one of the b'th root of a, such that |c|**b <= |a|
 *
 * returns error if a < 0 and b is even
 */
MP_WUR int mp_n_root(const mp_int *a, mp_digit b, mp_int *c);
MP_WUR int mp_n_root_ex(const mp_int *a, mp_digit b, mp_int *c, int fast);

/* special sqrt algo */
MP_WUR int mp_sqrt(const mp_int *arg, mp_int *ret);

/* special sqrt (mod prime) */
MP_WUR int mp_sqrtmod_prime(const mp_int *n, const mp_int *prime, mp_int *ret);

/* is number a square? */
MP_WUR int mp_is_square(const mp_int *arg, int *ret);

/* computes the jacobi c = (a | n) (or Legendre if b is prime)  */
MP_WUR int mp_jacobi(const mp_int *a, const mp_int *n, int *c);

/* computes the Kronecker symbol c = (a | p) (like jacobi() but with {a,p} in Z */
MP_WUR int mp_kronecker(const mp_int *a, const mp_int *p, int *c);

/* used to setup the Barrett reduction for a given modulus b */
MP_WUR int mp_reduce_setup(mp_int *a, const mp_int *b);

/* Barrett Reduction, computes a (mod b) with a precomputed value c
 *
 * Assumes that 0 < x <= m*m, note if 0 > x > -(m*m) then you can merely
 * compute the reduction as -1 * mp_reduce(mp_abs(x)) [pseudo code].
 */
MP_WUR int mp_reduce(mp_int *x, const mp_int *m, const mp_int *mu);

/* setups the montgomery reduction */
MP_WUR int mp_montgomery_setup(const mp_int *n, mp_digit *rho);

/* computes a = B**n mod b without division or multiplication useful for
 * normalizing numbers in a Montgomery system.
 */
MP_WUR int mp_montgomery_calc_normalization(mp_int *a, const mp_int *b);

/* computes x/R == x (mod N) via Montgomery Reduction */
MP_WUR int mp_montgomery_reduce(mp_int *x, const mp_int *n, mp_digit rho);

/* returns 1 if a is a valid DR modulus */
MP_WUR int mp_dr_is_modulus(const mp_int *a);

/* sets the value of "d" required for mp_dr_reduce */
void mp_dr_setup(const mp_int *a, mp_digit *d);

/* reduces a modulo n using the Diminished Radix method */
MP_WUR int mp_dr_reduce(mp_int *x, const mp_int *n, mp_digit k);

/* returns true if a can be reduced with mp_reduce_2k */
MP_WUR int mp_reduce_is_2k(const mp_int *a);

/* determines k value for 2k reduction */
MP_WUR int mp_reduce_2k_setup(const mp_int *a, mp_digit *d);

/* reduces a modulo b where b is of the form 2**p - k [0 <= a] */
MP_WUR int mp_reduce_2k(mp_int *a, const mp_int *n, mp_digit d);

/* returns true if a can be reduced with mp_reduce_2k_l */
MP_WUR int mp_reduce_is_2k_l(const mp_int *a);

/* determines k value for 2k reduction */
MP_WUR int mp_reduce_2k_setup_l(const mp_int *a, mp_int *d);

/* reduces a modulo b where b is of the form 2**p - k [0 <= a] */
MP_WUR int mp_reduce_2k_l(mp_int *a, const mp_int *n, const mp_int *d);

/* Y = G**X (mod P) */
MP_WUR int mp_exptmod(const mp_int *G, const mp_int *X, const mp_int *P, mp_int *Y);

/* ---> Primes <--- */

/* number of primes */
#ifdef MP_8BIT
#  define MP_PRIME_SIZE 31
#else
#  define MP_PRIME_SIZE 256
#endif
#define PRIME_SIZE (MP_DEPRECATED_PRAGMA("PRIME_SIZE has been deprecated, use MP_PRIME_SIZE") MP_PRIME_SIZE)

/* table of first MP_PRIME_SIZE primes */
extern const mp_digit ltm_prime_tab[MP_PRIME_SIZE];

/* result=1 if a is divisible by one of the first MP_PRIME_SIZE primes */
MP_WUR int mp_prime_is_divisible(const mp_int *a, int *result);

/* performs one Fermat test of "a" using base "b".
 * Sets result to 0 if composite or 1 if probable prime
 */
MP_WUR int mp_prime_fermat(const mp_int *a, const mp_int *b, int *result);

/* performs one Miller-Rabin test of "a" using base "b".
 * Sets result to 0 if composite or 1 if probable prime
 */
MP_WUR int mp_prime_miller_rabin(const mp_int *a, const mp_int *b, int *result);

/* This gives [for a given bit size] the number of trials required
 * such that Miller-Rabin gives a prob of failure lower than 2^-96
 */
MP_WUR int mp_prime_rabin_miller_trials(int size);

/* performs one strong Lucas-Selfridge test of "a".
 * Sets result to 0 if composite or 1 if probable prime
 */
MP_WUR int mp_prime_strong_lucas_selfridge(const mp_int *a, int *result);

/* performs one Frobenius test of "a" as described by Paul Underwood.
 * Sets result to 0 if composite or 1 if probable prime
 */
MP_WUR int mp_prime_frobenius_underwood(const mp_int *N, int *result);

/* performs t random rounds of Miller-Rabin on "a" additional to
 * bases 2 and 3.  Also performs an initial sieve of trial
 * division.  Determines if "a" is prime with probability
 * of error no more than (1/4)**t.
 * Both a strong Lucas-Selfridge to complete the BPSW test
 * and a separate Frobenius test are available at compile time.
 * With t<0 a deterministic test is run for primes up to
 * 318665857834031151167461. With t<13 (abs(t)-13) additional
 * tests with sequential small primes are run starting at 43.
 * Is Fips 186.4 compliant if called with t as computed by
 * mp_prime_rabin_miller_trials();
 *
 * Sets result to 1 if probably prime, 0 otherwise
 */
MP_WUR int mp_prime_is_prime(const mp_int *a, int t, int *result);

/* finds the next prime after the number "a" using "t" trials
 * of Miller-Rabin.
 *
 * bbs_style = 1 means the prime must be congruent to 3 mod 4
 */
MP_WUR int mp_prime_next_prime(mp_int *a, int t, int bbs_style);

/* makes a truly random prime of a given size (bytes),
 * call with bbs = 1 if you want it to be congruent to 3 mod 4
 *
 * You have to supply a callback which fills in a buffer with random bytes.  "dat" is a parameter you can
 * have passed to the callback (e.g. a state or something).  This function doesn't use "dat" itself
 * so it can be NULL
 *
 * The prime generated will be larger than 2^(8*size).
 */
#define mp_prime_random(a, t, size, bbs, cb, dat) (MP_DEPRECATED_PRAGMA("mp_prime_random has been deprecated, use mp_prime_rand instead") mp_prime_random_ex(a, t, ((size) * 8) + 1, (bbs==1)?MP_PRIME_BBS:0, cb, dat))

/* makes a truly random prime of a given size (bits),
 *
 * Flags are as follows:
 *
 *   MP_PRIME_BBS      - make prime congruent to 3 mod 4
 *   MP_PRIME_SAFE     - make sure (p-1)/2 is prime as well (implies MP_PRIME_BBS)
 *   MP_PRIME_2MSB_ON  - make the 2nd highest bit one
 *
 * You have to supply a callback which fills in a buffer with random bytes.  "dat" is a parameter you can
 * have passed to the callback (e.g. a state or something).  This function doesn't use "dat" itself
 * so it can be NULL
 *
 */
MP_WUR MP_DEPRECATED(mp_prime_rand) int mp_prime_random_ex(mp_int *a, int t, int size, int flags,
      private_mp_prime_callback cb, void *dat);
MP_WUR int mp_prime_rand(mp_int *a, int t, int size, int flags);

/* Integer logarithm to integer base */
MP_WUR int mp_ilogb(mp_int *a, mp_digit base, mp_int *c);


/* ---> radix conversion <--- */
MP_WUR int mp_count_bits(const mp_int *a);

MP_WUR int mp_unsigned_bin_size(const mp_int *a);
MP_WUR int mp_read_unsigned_bin(mp_int *a, const unsigned char *b, int c);
MP_WUR int mp_to_unsigned_bin(const mp_int *a, unsigned char *b);
MP_WUR int mp_to_unsigned_bin_n(const mp_int *a, unsigned char *b, unsigned long *outlen);

MP_WUR int mp_signed_bin_size(const mp_int *a);
MP_WUR int mp_read_signed_bin(mp_int *a, const unsigned char *b, int c);
MP_WUR int mp_to_signed_bin(const mp_int *a,  unsigned char *b);
MP_WUR int mp_to_signed_bin_n(const mp_int *a, unsigned char *b, unsigned long *outlen);

MP_WUR int mp_read_radix(mp_int *a, const char *str, int radix);
MP_WUR int mp_toradix(const mp_int *a, char *str, int radix);
MP_WUR int mp_toradix_n(const mp_int *a, char *str, int radix, int maxlen);
MP_WUR int mp_radix_size(const mp_int *a, int radix, int *size);

#ifndef MP_NO_FILE
MP_WUR int mp_fread(mp_int *a, int radix, FILE *stream);
MP_WUR int mp_fwrite(const mp_int *a, int radix, FILE *stream);
#endif

#define mp_read_raw(mp, str, len) (MP_DEPRECATED_PRAGMA("replaced by mp_read_signed_bin") mp_read_signed_bin((mp), (str), (len)))
#define mp_raw_size(mp)           (MP_DEPRECATED_PRAGMA("replaced by mp_signed_bin_size") mp_signed_bin_size(mp))
#define mp_toraw(mp, str)         (MP_DEPRECATED_PRAGMA("replaced by mp_to_signed_bin") mp_to_signed_bin((mp), (str)))
#define mp_read_mag(mp, str, len) (MP_DEPRECATED_PRAGMA("replaced by mp_read_unsigned_bin") mp_read_unsigned_bin((mp), (str), (len))
#define mp_mag_size(mp)           (MP_DEPRECATED_PRAGMA("replaced by mp_unsigned_bin_size") mp_unsigned_bin_size(mp))
#define mp_tomag(mp, str)         (MP_DEPRECATED_PRAGMA("replaced by mp_to_unsigned_bin") mp_to_unsigned_bin((mp), (str)))

#define mp_tobinary(M, S)  mp_toradix((M), (S), 2)
#define mp_tooctal(M, S)   mp_toradix((M), (S), 8)
#define mp_todecimal(M, S) mp_toradix((M), (S), 10)
#define mp_tohex(M, S)     mp_toradix((M), (S), 16)

#ifdef __cplusplus
}
#endif

#endif
