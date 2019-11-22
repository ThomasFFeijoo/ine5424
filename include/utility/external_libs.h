// Source code from external libs

#include <system/config.h>

#ifndef __external_libs_h
#define __external_libs_h

__BEGIN_UTIL

class Helper {

public:
    Helper() {}

    // src: https://stackoverflow.com/a/2284929
    double sin(double x){
        int i = 1;
        double cur = x;
        double acc = 1;
        double fact= 1;
        double pow = x;
        while (fabs(acc) > .00000001 &&   i < 100){
            fact *= ((2*i)*(2*i+1));
            pow *= -1 * x*x;
            acc =  pow / fact;
            cur += acc;
            i++;
        }
        return cur;
    }

    // src: https://stackoverflow.com/a/2284969
    double cos(double x) {
        double t, s ;
        int p;
        p = 0;
        s = 1.0;
        t = 1.0;
        while(fabs(t/s) > .00000001) {
            p++;
            t = (-t * x * x) / ((2 * p - 1) * (2 * p));
            s += t;
        }
        return s;
    }

    // src: http://beedub.com/Sprite093/src/lib/c/stdlib/atof.c
    /*
    *----------------------------------------------------------------------
    *
    * atof --
    *
    *	This procedure converts a floating-point number from an ASCII
    *	decimal representation to internal double-precision format.
    *
    * Results:
    *	The return value is the floating-point equivalent of string.
    *	If a terminating character is found before any floating-point
    *	digits, then zero is returned.
    *
    * Side effects:
    *	None.
    *
    *----------------------------------------------------------------------
    */
    double atof(char *string)
                    /* A decimal ASCII floating-point number,
                    * optionally preceded by white space.
                    * Must have form "-I.FE-X", where I is the
                    * integer part of the mantissa, F is the
                    * fractional part of the mantissa, and X
                    * is the exponent.  Either of the signs
                    * may be "+", "-", or omitted.  Either I
                    * or F may be omitted, or both.  The decimal
                    * point isn't necessary unless F is present.
                    * The "E" may actually be an "e".  E and X
                    * may both be omitted (but not just one).
                    */
    {
        double powersOf10[] = {	/* Table giving binary powers of 10.  Entry */
            10.,			    /* is 10^2^i.  Used to convert decimal */
            100.,			    /* exponents into floating-point numbers. */
            1.0e4,
            1.0e8,
            1.0e16,
            1.0e32,
            1.0e64,
            1.0e128,
            1.0e256
        };

        int sign, expSign = 0;
        double fraction, dblExp, *d;
        register char *p, c;
        int exp = 0;		/* Exponent read from "EX" field. */
        int fracExp = 0;		/* Exponent that derives from the fractional
                    * part.  Under normal circumstatnces, it is
                    * the negative of the number of digits in F.
                    * However, if I is very long, the last digits
                    * of I get dropped (otherwise a long I with a
                    * large negative exponent could cause an
                    * unnecessary overflow on I alone).  In this
                    * case, fracExp is incremented one for each
                    * dropped digit.
                    */
        int mantSize;		/* Number of digits in mantissa. */
        int decPt;			/* Number of mantissa digits BEFORE decimal
                    * point.
                    */
        char *pExp;			/* Temporarily holds location of exponent
                    * in string.
                    */

        /*
        * Strip off leading blanks and check for a sign.
        */

        p = string;
        while (isspace(*p)) {
            p += 1;
        }
        if (*p == '-') {
            sign = 	1;
            p += 1;
        } else {
            if (*p == '+') {
                p += 1;
            }
            sign = 0;
        }

        /*
        * Count the number of digits in the mantissa (including the decimal
        * point), and also locate the decimal point.
        */

        decPt = -1;
        for (mantSize = 0; ; mantSize += 1)
        {
            c = *p;
            if (!isdigit(c)) {
                if ((c != '.') || (decPt >= 0)) {
                    break;
                }
                decPt = mantSize;
            }
            p += 1;
        }

        /*
        * Now suck up the digits in the mantissa.  Use two integers to
        * collect 9 digits each (this is faster than using floating-point).
        * If the mantissa has more than 18 digits, ignore the extras, since
        * they can't affect the value anyway.
        */

        pExp  = p;
        p -= mantSize;
        if (decPt < 0) {
            decPt = mantSize;
        } else {
            mantSize -= 1;			/* One of the digits was the point. */
        }
        if (mantSize > 18) {
            fracExp = decPt - 18;
            mantSize = 18;
        } else {
            fracExp = decPt - mantSize;
        }
        if (mantSize == 0) {
            return 0.0;
        } else {
            int frac1, frac2;
            frac1 = 0;
            for ( ; mantSize > 9; mantSize -= 1)
            {
                c = *p;
                p += 1;
                if (c == '.') {
                c = *p;
                p += 1;
                }
                frac1 = 10*frac1 + (c - '0');
            }
            frac2 = 0;
            for (; mantSize > 0; mantSize -= 1)
            {
                c = *p;
                p += 1;
                if (c == '.') {
                c = *p;
                p += 1;
                }
                frac2 = 10*frac2 + (c - '0');
            }
            fraction = (1.0e9 * frac1) + frac2;
        }

        /*
        * Skim off the exponent.
        */

        p = pExp;
        if ((*p == 'E') || (*p == 'e')) {
            p += 1;
            if (*p == '-') {
                expSign = 1;
                p += 1;
            } else {
                if (*p == '+') {
                p += 1;
                }
                expSign = 0;
            }
            while (isdigit(*p)) {
                exp = exp * 10 + (*p - '0');
                p += 1;
            }
        }
        if (expSign) {
            exp = fracExp - exp;
        } else {
            exp = fracExp + exp;
        }

        /*
        * Generate a floating-point number that represents the exponent.
        * Do this by processing the exponent one bit at a time to combine
        * many powers of 2 of 10. Then combine the exponent with the
        * fraction.
        */

        if (exp < 0) {
            expSign = 1;
            exp = -exp;
        } else {
            expSign = 0;
        }
        if (exp > maxExponent) {
            exp = maxExponent;
        }
        dblExp = 1.0;
        for (d = powersOf10; exp != 0; exp >>= 1, d += 1) {
            if (exp & 01) {
                dblExp *= *d;
            }
        }
        if (expSign) {
            fraction /= dblExp;
        } else {
            fraction *= dblExp;
        }

        if (sign) {
            return -fraction;
        }
        return fraction;
    }

// src: http://beedub.com/Sprite093/src/lib/c/stdlib/atof.c
private:
    static const int maxExponent = 511;	/* Largest possible base 10 exponent.  Any
                                     * exponent larger than this will already
				                     * produce underflow or overflow, so there's
				                     * no need to worry about additional digits.
				                     */

    bool isspace(unsigned char c);
    bool isdigit(unsigned char c);
    double fabs(double x);

};

__END_UTIL

#endif
