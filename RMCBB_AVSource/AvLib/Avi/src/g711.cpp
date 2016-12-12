/*
 * g711.cpp
 *
 *  Created on: 2012-6-20
 *      Author: gami
 */

#define	SIGN_BIT	(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK	(0xf)		/* Quantization field mask. */
#define	NSEGS		(8)		   /* Number of A-law segments. */
#define	SEG_SHIFT	(4)		   /* Left shift for segment number. */
#define	SEG_MASK	(0x70)		/* Segment field mask. */

#define	BIAS		(0x84)		/* Bias for linear code. */
#define CLIP            8159

/*
 * alaw2linear() - Convert an A-law value to 16-bit linear PCM
 *
 */
int alaw2linear(int	a_val)
{
	int		t;      /* changed from "short" *drago* */
	int		seg;    /* changed from "short" *drago* */

	a_val ^= 0x55;

	t = (a_val & QUANT_MASK) << 4;
	seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
	switch (seg) {
	case 0:
		t += 8;
		break;
	case 1:
		t += 0x108;
		break;
	default:
		t += 0x108;
		t <<= seg - 1;
	}
	return ((a_val & SIGN_BIT) ? t : -t);
}

/*
 * ulaw2linear() - Convert a u-law value to 16-bit linear PCM
 *
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with ISDN conventions.
 */
int ulaw2linear( int	u_val)
{
	int t;

	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;

	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= (u_val & SEG_MASK) >> SEG_SHIFT;

	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}


int G711AlawDecoder(char indata[], short outdata[], int len)
{
	char data;

    for(int i=0; i<len; i++)
    {
        data = indata[i];

        outdata[i] = (short)alaw2linear((int)data);
    }

    return len*2;
}

int G711UlawDecoder(char indata[], short outdata[], int len)
{
	char data;

    for(int i=0; i<len; i++)
    {
        data = indata[i];

        outdata[i] = (short)ulaw2linear((int)data);
    }

    return len*2;
}


