precision mediump float;
attribute vec2 a;
varying vec2 uv;
void main() {
    uv = (a + vec2(1.0)) * 0.5;
    gl_Position = vec4(a, 0.0, 1.0);
}
    