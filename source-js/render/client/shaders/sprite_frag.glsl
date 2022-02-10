precision highp float;

varying vec2 uv;
varying vec4 color;
varying float tex;
uniform sampler2D s[16];

void main() {
    vec4 c;
    if(tex < 0.5) {
        c = texture2D(s[0], uv);
    } else if(tex < 1.5) {
        c = texture2D(s[1], uv);
    } else if(tex < 2.5) {
        c = texture2D(s[2], uv);
    } else if(tex < 3.5) {
        c = texture2D(s[3], uv);
    } else if(tex < 4.5) {
        c = texture2D(s[4], uv);
    } else if(tex < 5.5) {
        c = texture2D(s[5], uv);
    } else if(tex < 6.5) {
        c = texture2D(s[6], uv);
    } else if(tex < 7.5) {
        c = texture2D(s[7], uv);
    } else if(tex < 8.5) {
        c = texture2D(s[8], uv);
    } else if(tex < 9.5) {
        c = texture2D(s[9], uv);
    } else if(tex < 10.5) {
        c = texture2D(s[10], uv);
    } else if(tex < 11.5) {
        c = texture2D(s[11], uv);
    } else if(tex < 12.5) {
        c = texture2D(s[12], uv);
    } else if(tex < 13.5) {
        c = texture2D(s[13], uv);
    } else if(tex < 14.5) {
        c = texture2D(s[14], uv);
    } else {
        c = texture2D(s[15], uv);
    }
    // gl_FragColor = vec4(uv, 1.0, 1.0);
    gl_FragColor = c * color;
}