//backdrop vertex shader - transforms and textures

vs.1.1
dcl_position v0
dcl_texcoord0 v1

//transform vertex position
dp4 oPos.x, v0, c0
dp4 oPos.y, v0, c1
dp4 oPos.z, v0, c2
dp4 oPos.w, v0, c3

//store texture coords
mov oT0, v1
