//static geometry vertex shader - Pass 1: Ambient lighting

//c0..3	transposed world-view-projection transformation matrix
//c4..7	inverse-transpose world transformation matrix
//c8	diffuse vertex colour
//c9	ambient light colour

vs.1.1
dcl_position	v0
dcl_normal		v1

//transform vertex position
dp4 oPos.x, v0, c0
dp4 oPos.y, v0, c1
dp4 oPos.z, v0, c2
dp4 oPos.w, v0, c3

//ambient component
mov r0, c9
mul oD0, r0, c8
