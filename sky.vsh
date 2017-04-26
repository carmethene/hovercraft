//sky vertex shader - procedural clouds

//c0..3	transposed world-view-projection transformation matrix
//c4	current time

vs.1.1
dcl_position v0

def c5, 0.005f,0.005f,0.0f,0.0f

//transform vertex position
dp4 oPos.x, v0, c0
dp4 oPos.y, v0, c1
dp4 oPos.z, v0, c2
dp4 oPos.w, v0, c3

//create texture coords
mov r7.x, v0.x
mov r7.y, v0.z
mul r7.xy, r7.xy, c5.xy

//peturb texture coordinates by the time...
//first set
frc r0.xy, c4.x
add oT0.xy, r7, r0
add oT1.xy, r7, -r0

//second set
mov r0.x, -c4.y
mov r0.y, c4.y
frc r0.xy, r0
add oT2.xy, r7, -r0
