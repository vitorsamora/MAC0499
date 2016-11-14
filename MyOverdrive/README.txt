Overdrive with variable TH
-------------------------------

Overdrive waveshaping with TH parameter, where 0 < TH < 0.5.

We solve a linear system to find the coefficients of the waveshaping function:
	-> l(x) = ax + b
	-> q(x) = cx^2 + dx + e

	{l(x)	when x < TH
f(x) =	{q(x)	when TH <= x <= 2*TH
	{1	when x > 2*TH

With the constraints:
	-> l(TH) = q(TH)
	-> q(2*TH) = 1
	-> l'(TH) = q'(TH)
	-> q'(2*TH) = 0 (This is not true for DAFX overdrive)

Setting b = 0, we have:
	-> a = 2/((2 + TH) * TH)
	-> b = 0
	-> c = -1/((2 + TH) * TH)
	-> d = 4/(2 + TH)
	-> e = (2 - 3*TH)/(2 + TH)
