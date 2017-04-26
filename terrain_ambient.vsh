//terrain vertex shader - pass 1: calculates ambient colour and
//texture blending value

//c0..3	transposed world-view-projection transformation matrix
//c4	ambient light colour

vs.1.1
dcl_position	v0
dcl_normal		v1
dcl_color		v2
dcl_texcoord0	v3
dcl_texcoord1	v4

//transformed vertex position
dp4 oPos.x, v0, c0
dp4 oPos.y, v0, c1
dp4 oPos.z, v0, c2
dp4 oPos.w, v0, c3

//texture coordinates
mov oT0, v3.xy
mov oT1, v4.xy

//blending value for textures (y component of vertex normal)
mov oT2, v1.y

//ambient component
mov oD0, c4
