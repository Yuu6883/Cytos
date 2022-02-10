precision highp float;

uniform float time;
uniform sampler2D tex;
uniform vec3 color;

varying vec2 v_uv;

float det = 0.005, maxdist = 50.0, pi = 3.1416, gl = 0.0;
vec2 id;

float hash12(vec2 p) {
    p *= 1000.0;
    vec3 p3  = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, s, -s, c);
}

float box(vec3 p, vec3 c) {
    vec3 pc = abs(p) - c;
    return length(max(vec3(0.0), pc)) - min(0.0, max(pc.z, max(pc.x, pc.y)));
}

vec2 amod(vec2 p, float n, float off, out float i) {
    float l=length(p)-off;
    float at=atan(p.x,p.y)/pi*n*.5;
    i=abs(floor(at));
    float a=fract(at)-.5;
    return vec2(a,l);
}

float ring(vec3 p,inout vec2 id) {
    p.xy = amod(p.xy * rot(time * 0.0), 20., 2., id.x);
    float h=max(0.0, texture2D(tex, vec2(.5 + fract(id.x*.2+id.y*.1), 0.)).r * 5.0);
    h += sin(time * 2. + id.x) * .2;
    float d=box(p+vec3(0.,-h*1.5,0.),vec3(.1,h,.1));
    return d*.5;
}

float de(vec3 p) {
    float d = 100.0, ii = 0.0;
    
    p.xz *= rot(time * 0.05);
    p.yz *= rot(sin(time * 0.05));

    vec2 ids;
    for (int i = 0; i < 4; i++) {
        p.xz *= rot(pi * 0.25);
        ids.y = float(i);
        // float r = ring(p, ids);
        // if (r < d) {
        //     d = r;
        //     id = ids;
        // }
    }
    d = min(d, length(p) - 1.5);
    return d * 0.7;
}

vec3 normal(vec3 p) {
    vec2 e=vec2(0., det);
    return normalize(vec3(de(p + e.yxx), de(p + e.xyx), de(p + e.xxy)) - de(p));
}

vec3 march(vec3 from, vec3 dir) {
    float d, td = 0.0;
    vec3 p, col = vec3(0.0);

    for (int i = 0; i < 10; i++) {
        p = from + td * dir;
        d = de(p);
        if (d < det || td > maxdist) break;
        td += d;
        gl += 0.1 / (10.0 + d * d * 10.0) * step(0.7, hash12(id + floor(time * 5.0)));
    }

    if (d < det) {
        float h = hash12(id);
        vec3 colid = mix(color, vec3(1, 1, 1), h);
        p -= dir * det;
        vec3 n = normal(p);
        vec2 e = vec2(0.0, 0.05);
        col = 0.1 + max(0.0, dot(-dir, n)) * colid;
    } else {
        dir.xz *= rot(sin(time * 0.5) * 0.15);
        dir.yz *= rot(cos(time * 0.5) * 0.15);
        vec2 p2 = abs(0.5 - fract(dir.yz));
        float d2 = 100.0, is = 0.0;
        for (int i = 0; i < 10; i++) {
            p2 = abs(p2 * 1.3) * rot(radians(45.0)) - 0.5;
            float sh = length(max(vec2(0.), abs(p2) - 0.05));
            if (sh < d2) {
                d2 = sh;
                is = float(i);
            }
        }
        col += smoothstep(0.05, 0.0, d2) * fract(is * 0.1 + time * 0.15) * normalize(p + 50.0);
    }
    return col + gl;
}

void main() {
    vec2 uv = v_uv * 2.0;
    vec3 from = vec3(0.0, 0.0, -8.0);
    vec3 dir = normalize(vec3(uv, 0.7));
    vec3 col = march(from, dir);
    gl_FragColor = vec4(col, 1.0);
}