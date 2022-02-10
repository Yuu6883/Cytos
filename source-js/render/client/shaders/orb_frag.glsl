// https://www.shadertoy.com/view/Wsy3zm Modified
precision highp float;

uniform float time;
varying vec2 v_uv;

mat2 rot(float a) {
    float ca = cos(a);
    float sa = sin(a);
    return mat2(ca, sa, -sa, ca);    
}

float smin(float a, float b, float h) {
    float k = clamp((a - b) / h * 0.5 + 0.5, 0.0, 1.0);
    return mix(a, b, k) - k * (1.0 - k) * h;
}

vec3 smin(vec3 a, vec3 b, float h) {
    vec3 k = clamp((a - b) / h * 0.5 + 0.5, 0.0, 1.0);
    return mix(a, b, k) - k * (1.0 - k) * h;
}

float rnd(float a) {
    return fract(sin(a * 452.655) * 387.521);    
}

vec3 rnd3(float a) {
    return fract(sin(a * vec3(845.652, 257.541, 289.675)) * vec3(354.852, 685.527, 947.544));
}

vec3 rndcol(float a) {
    vec3 r = rnd3(a);
    r /= max(r.x, max(r.y,r.z));
    return r;
}

float rnd(vec2 uv) {
    return fract(dot(sin(uv * 357.542 + uv.yx * 685.427), vec2(925.674)));
}

vec3 kifs(vec3 p, float t, float t3) {
    vec3 bp = p;
    float s = 0.0 + smoothstep(0.0, 1.0, fract(t3)) * 10.0;
    for(int i = 0; i < 3; ++i) {
        float t2 = t + float(i);
        p.xz *= rot(t2);
        p.xy *= rot(t2*0.7);
        // using smooth minimum on all 3 channels to make a "smooth" symmetry
        p = smin(p, -p, -3.0);
        //p=abs(p);
        p -= s;// + sin(t*0.2-length(p)*1.0) * 0.5;
        s *= 0.7;
    }
    return p;
}

float exspeed = 0.25;
float explode(vec3 p, float t, float offset) {
    
    // rotating and swirling in all directions
    p.xz *= rot(p.y*0.09+t*0.15);
    p.xy *= rot(p.z*0.052+t*0.18);
    
    float t1 = t * exspeed + offset;
    vec3 p2 = kifs(p, t*0.1, t1);
    vec3 p3 = kifs(p + vec3(3,2,1), t * 0.13, t1);
    
    float fade = 1.0 - pow(fract(t1), 10.0);
    
    float d1 = length(p2)- 1.5 * fade;
    d1 = min(d1, length(p2.xz) - 0.8 * fade);
    float d2 = length(p3) - 1.4 * fade;
    d2 = min(d2, length(p3.xz) - 0.8 * fade);
    
    return smin(d1,d2, -1.0);
}

float box(vec3 p, float r) {
    p = abs(p)-r;
    return max(p.x, max(p.y,p.z));
}

float box(vec2 p, float r) {
    p = abs(p) - r;
    return max(p.x, p.y);
}

float cyl(vec2 p, float r) {
    return length(p) - r;
}

float at1 = 0.0;
float at2 = 0.0;

vec3 pos1 = vec3(0);
vec3 pos2 = vec3(0);

float map(vec3 p) {
    // we compute two explosion at the same time
    // offseted in time so one explosion start when the other disappear
    float m1 = explode(p + pos1, time, 0.0);
    float m2 = explode(p + pos2, time, 0.5);
    
    at1 += 0.8 / (2.0 + abs(m1));
    at2 += 0.8 / (2.0 + abs(m2));
    
    float m3 = min(abs(m1), abs(m2));
    
    vec3 bp = p;
    bp += smoothstep(-0.5, 0.5, sin(p.yzx * 1.0)) * 0.2;
    float other = (length(bp) - 15.0) * 0.8;
    
    return min(m3, other);
}

void cam(inout vec3 p) {
    float t = time*0.3;
    p.xz *= rot(t);
    p.xy *= rot(t*1.2);
}

void main() {
        
    vec2 uv = v_uv;
    
    // pick a random position for each explosion
    pos1 = (rnd3(floor(time * exspeed)) - 0.5) * 20.0;
    pos2 = (rnd3(floor(time * exspeed + 0.5) + 37.2) -0.5) * 20.0;
    
    float fade1 = 1.0 - pow(fract(time * exspeed),10.0);
    float fade2 = 1.0 - pow(fract(time * exspeed + 0.5),10.0);
    
    // pick a random color for each explosion
    // vec3 c1 = rndcol(floor(time*exspeed)+17.3);
    // vec3 c2 = rndcol(floor(time*exspeed+0.5)+37.5);
    vec4 c1 = vec4(0.64, 0.35, 0.86, 1.0);
    vec4 c2 = vec4(0.64, 0.35, 0.86, 1.0);
    
    vec3 s = vec3(0, 0, -60);
    vec3 r = normalize(vec3(-uv, 1));
    
    cam(s);
    cam(r);
    
    float dither = mix(1.0, rnd(uv), 0.1);
    
    vec2 off = vec2(0.01,0);
    
    vec4 col = vec4(0);
    vec3 p = s;
    
    for (int i= 0; i < 80; ++i) {
        float d = max(map(p), 0.0) * dither;
        if(d < 0.01) {
            // instead of breaking when we find a surface, we continue to march forward with a slight offset
            d = 0.1;
        }
        col += pow(at1 * 0.010, 2.5) * c1 * fade1;
        col += pow(at2 * 0.010, 2.5) * c2 * fade2;
        p += r * d;
    }

    col *= 3.0;
    
    // the dark souls
    col *= smoothstep(0.0, 0.4, length(uv) + 0.05);
    
    // bloom to white on primary colors
    col += max(vec4(0), col.yzxw - 1.0);
    col += max(vec4(0), col.zxyw - 1.0);
    
    col = 1.0 - exp(-col);
    col = pow(col, vec4(1.0));
    
    gl_FragColor = col;
}