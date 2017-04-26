// vehicle vertex shader

// c0..3	transposed world-view-projection transformation matrix
// c4..7	inverse-transpose world transformation matrix
// c8		ambient light colour
// c9		use lights?
// c10,11	light direction / colour
// c20		(0, 0, 0, 0)

vs.2.0
dcl_position	v0
dcl_normal		v1

def c20, 0.0f,0.0f,0.0f,0.0f

//transform vertex position
dp4 oPos.x, v0, c0
dp4 oPos.y, v0, c1
dp4 oPos.z, v0, c2
dp4 oPos.w, v0, c3

//transform vertex normals
dp4 r5.x, v1, c4
dp4 r5.y, v1, c5
dp4 r5.z, v1, c6
dp4 r5.w, v1, c7

//calculate vertex colour...
//store light vector
nrm r0, -c10

//diffuse component
dp3 r1, r0, r5
max r1, r1, c20	//no negative colour vals
mul r2, r1, c9

//light colour
mul r2, r2, c11

//ambient component
add oD0, r2, c8
