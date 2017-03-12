Name: Wilson Yeh
Email: wilsonye@usc.edu
ID: 4780838615

Select the spline by changing the file in track.txt
Run it and enjoy the ride!

Click and drag (use right-click for z-axis)
	'Rotate' => No Modifier Key
	'Scale' => Shift
	'Translate' => Control

Press 'x' to take a screenshot
Press the spacebar to start/stop recording (continuous screenshots)

Extra Credit:
1. Render double rail
2. More realistic roller coaster velocity
3. Derive roller coaster velocity equation
	Potential Energy: mgh
	Kinetic Energy: (1/2)mv^2
	Energy is conserved, potential energy is maximum at h-max
	Potential Energy = Kinetic Energy
	Amount of potential energy converted into kinetic energy at height h is mg(h-max) - mgh = mg(h-max - h)
	Kinetic energy at height h, then, is
	mg(h-max - h) = (1/2)mv^2
	g(h-max - h) = (1/2)v^2
	v^2 = 2g(h-max - h)
	v = sqrt(2g(h-max - h)) // Speed

	If p is the position function of u, then dp/du is the velocity function of u, use it to give v direction

	By definition,
	position-new = position-old + [delta-time * velocity]

	Substitution,
	u-new = u-current + [(delta-t) * v/||dp/du||]
	u-new = u-current + [(delta-t) * sqrt(2g(h-max - h)) / ||dp/du||]