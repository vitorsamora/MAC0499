Overdrive with 2 variable THs
-------------------------------

Overdrive waveshaping with two TH parameters, where 0 < TH_L < TH_R < 1.

We solve a linear system to find the coefficients of the waveshaping function:
	-> l(x) = ax + b
	-> q(x) = cx^2 + dx + e

	{l(x)	when x < TH_L
f(x) =	{q(x)	when TH_L <= x <= TH_R
	{1	when x > TH_R

With the constraints:
	-> l(TH_L) = q(TH_L)
	-> q(TH_R) = 1
	-> l'(TH_L) = q'(TH_L)
	-> q'(TH_R) = 0

c*(TH_L)² + (d - a)*(TH_L) + (e - b) = 0
c*(TH_R)² + d*(TH_R) + e = 1
2*c*(TH_L) + (d - a) = 0
2*c*(TH_R) + d = 0
	
	||
    	\/

|-(TH_L)		-1	(TH_L)²		(TH_L)		1| |a|		
|0			0	(TH_R)²		(TH_R)		1| |b|		
|-1			0	2*(TH_L)	1		0| |c| 	= 	|0	1	0	0|
|0			0	2*(TH_R)	1		0| |d|		
|			    	 				 | |e|


Taking b as a parameter:
	-> e = (TH_L² - 2*b*(TH_R - TH_R²/2))/(TH_L² - 2*(TH_R - TH_R²/2));
	-> d = 2*(b - e)/TH_L²;
	-> c = -d/2;
	-> a = d + 2*TH_L*c;

With the constraints:
	-> TH_L² != 0 => TH_L != 0;  -> OK
	-> TH_L² - 2*(TH_R - TH_R²/2) != 0 <=> TH_R² - 2*TH_R + TH_L² != 0 
				<=> TH_R != 1 - sqrt(1 - TH_L²) -> Need to check!                                       )
