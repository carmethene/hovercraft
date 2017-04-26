//shadow volume vertex shader - simply transforms and outputs a
//constant colour

//c0..3	transposed world-view-projection transformation matrix

vs.1.1
dcl_position v0

def c4, 0.6f, 0.6f, 0.0f, 0.0f	//shadow volume colour

//transform vertex position
dp4 oPos.x, v0, c0
dp4 oPos.y, v0, c1
dp4 oPos.z, v0, c2
dp4 oPos.w, v0, c3

//output vertex colour
mov oD0, c4
