precision highp float;

varying vec2 uv;
varying vec4 color;
varying float tex;
uniform sampler2D s;

float circle(in vec2 _st, in float _radius){
    vec2 dist = _st - vec2(0.5);
	return 1.0 - smoothstep(_radius-(_radius * 0.01),
                         _radius+(_radius * 0.01),
                         dot(dist,dist) * 4.0);
}

void main() {
    vec4 c = texture2D(s, uv);
    gl_FragColor = c * color * circle(uv, 0.95);
    // gl_FragColor = vec4(uv, 1.0, 1.0) * circle(uv, 0.9);
}