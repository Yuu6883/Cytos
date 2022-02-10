precision highp float;

attribute vec2 p;

varying vec2 v_uv;

void main() {
    v_uv = p;
    gl_Position = vec4(p, 0.0, 1.0);
}