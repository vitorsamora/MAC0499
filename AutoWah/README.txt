Auto-wah
-------------------------------

Second-order allpass: A(z)

	fc -> analog cut-off frequency
	fs -> sampling frequency
	fb -> bandwidth

The parameter d adjusts the cut-off frequency and the parameter c the bandwidth:
	d = -cos(2*PI*fc/fs)
	c = (tan(PI*fb/fs) - 1)/(tan(2*PI*fb/fs) + 1)

	y(n) = -c * x(n) + d * (1 - c) * x(n-1) + x(n-2) - d * (1 - c) * y(n - 1) + c * y(n - 2)

Second-order bandpass:
	H(z) = 0.5 * (1 + A(z))

Wah-wah: Change fc over time in a second-order bandpass with small bandwidth

Auto-wah: Change fc according to maximum value of input block
