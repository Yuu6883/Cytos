precision highp float;

attribute vec2 v;
attribute vec2 u;
attribute vec4 c;
attribute float t;

uniform mat4 p;

varying vec2 uv;
varying vec4 color;
varying float tex;

void main() {
    gl_Position = p * vec4(v, 0.0, 1.0);
    uv = u;
    color = c;
    tex = t;
}