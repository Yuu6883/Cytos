precision highp float;

uniform mat4 p;

attribute vec2 a;
attribute vec2 u;

varying vec2 v_uv;

void main() {
    v_uv = u;
    gl_Position = p * vec4(a, 0.0, 1.0);
}