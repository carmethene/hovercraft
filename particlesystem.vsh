//particle system vertex shader - simply transforms and outputs a
//constant colour

//c0..3	transposed world-view-projection transformation matrix

vs.1.1
dcl_position v0

//def c4, 0.063f, 0.052f, 0.025f, 0.0f	//particle colour
def c4, 0.083f, 0.072f, 0.045f, 0.0f	//particle colour
def c5, 3.5f, 0.0f, 0.0f, 0.0f	//particle size

//transform vertex position
dp4 oPos.x, v0, c0
dp4 oPos.y, v0, c1
dp4 oPos.z, v0, c2
dp4 oPos.w, v0, c3

//particle size
mov oPts, c5.x

//output vertex colour
mov oD0, c4
