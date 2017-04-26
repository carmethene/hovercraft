//static geoemtry vertex shader - Pass 2: Diffuse lighting

//c0..3	transposed world-view-projection transformation matrix
//c4..7	inverse-transpose world transformation matrix
//c8	diffuse vertex colour
//c9,10	light direction / colour
//c11	(0, 0, 0, 0)

vs.1.1
dcl_position	v0
dcl_normal		v1

def c11, 0.0f,0.0f,0.0f,0.0f

//transformed vertex position
dp4 oPos.x, v0, c0
dp4 oPos.y, v0, c1
dp4 oPos.z, v0, c2
dp4 oPos.w, v0, c3

//transformed vertex normal
dp4 r5.x, v1, c4
dp4 r5.y, v1, c5
dp4 r5.z, v1, c6
dp4 r5.w, v1, c7

//diffuse component
dp3 r0, -c9, r5
max r0, r0, c11	//no negative colour vals
mul r0, r0, c10
mul oD0, r0, c8
