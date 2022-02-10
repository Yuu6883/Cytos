attribute vec2 aVertexPosition;
        uniform mat3 projectionMatrix;
        varying vec2 vTextureCoord;
        uniform vec4 outputFrame;
        vec4 filterVertexPosition( void )
        {
            vec2 position = aVertexPosition * max(outputFrame.zw, vec2(0.)) + outputFrame.xy;
            return vec4((projectionMatrix * vec3(position, 1.0)).xy, 0.0, 1.0);
        }
        void main(void)
        {
            gl_Position = filterVertexPosition();
            vTextureCoord = aVertexPosition;
        }
        `, `
        varying vec2 vTextureCoord;
        uniform sampler2D uSampler;
        uniform sampler2D uAudio;
        uniform vec2 iResolution;
        uniform vec2 map;
        uniform vec3 camera;
        uniform float iTime;
        #define PI 3.1415926535
        #define TAU 6.283185307
        // Tweak these constants depending
        // on the song...
        #define SCALE_REACTIVENESS 1.0
        #define FREQUENCY_REACTIVENESS 1.0
        #define HIGHS_MULTIPLIER 2.0
        #define SPECTRUM_SCALE 2.0
        float getFrequencies(in vec2 uv)
        {
        	return texture2D(uAudio, vec2(uv.x, 0.25)).r;
        }
        float getLowEnd()
        {
        	return texture2D(uAudio, vec2(0.2, 0.25)).r;
        }
        vec3 getCircleColor(in vec2 st)
        {
            float xCol = (st.x - (iTime / 13.0)) * 3.0;
        	xCol = mod(xCol, 3.0);
        	vec3 color = vec3(0.25, 0.25, 0.25);
        	if (xCol < 1.0) {
                color.g += 1.0 - xCol;
                color.b += xCol;
        	}
        	else if (xCol < 2.0) {
        		xCol -= 1.0;
        		color.b += 1.0 - xCol;
        		color.r += xCol;
        	}
        	else {
        		xCol -= 2.0;
        		color.r += 1.0 - xCol;
        		color.g += xCol;
        	}
            return color;
        }
        vec3 getColorGradient(in vec2 uv, vec3 col1, vec3 col2, float speed, float scale)
        {
        	vec3 col;
            uv *= scale;
            float s = (sin((uv.x + iTime / 5.0 * speed + uv.y + cos(uv.x + (iTime / 7.0 * speed) * 3.0 + sin((uv.y + iTime * speed / 10.0) * 5.0))) * 14.0) + 1.0) / 2.0;
            col = mix(col1, col2, s);
            return col;
        }
        float vignette(in vec2 uv, float borderStrength, float strength)
        {
        	return mix(1.0, pow(sin(uv.x * PI) * sin(uv.y * PI), borderStrength), strength);
        }
        void main()
        {
            vec2 camDim = iResolution / camera.z;
            vec2 world = (vTextureCoord - vec2(0.5, 0.5)) * camDim + camera.xy;
            if (world.x > map.x || world.x < -map.x ||
                world.y > map.y || world.y < -map.y) discard;
            world.y = -world.y;
            vec2 p = world / map;
            vec2 uv = (p + vec2(1.0, 1.0)) / 2.0;
            float a = atan(p.x, p.y);
            float r = length(p) * 5.0;
            vec2 st = vec2(a / TAU, r);
            vec3 col;
            vec3 spectrumColor = getCircleColor(st);
            float freq;
            freq += getFrequencies(st / 4.0) * step(1.0 - uv.x, 0.5);
            freq += getFrequencies(st / 4.0 * vec2(-1.0, 1.0)) * step(uv.x, 0.5);
            freq = clamp(freq, 0.0, 1.0);
            freq = pow(clamp(freq, 0.0, 1.0), 3.0);
            vec2 z = st * vec2(-1.0, 1.0);
            float freqMultiplier = pow((clamp(st.x, 0.0, 1.0) + clamp(z.x, 0.0, 1.0)) * 2.0, HIGHS_MULTIPLIER) + 0.15;
            freq *= freqMultiplier * 4.0;
            float lowEnd = getLowEnd() * SCALE_REACTIVENESS;
            st = ((1.85 * SPECTRUM_SCALE + lowEnd) + (freq * FREQUENCY_REACTIVENESS - 2.8) * st) - 1.0;
            float spectrum = 1.7 * abs(1.0 / (60.0 * st.y));
            float brightnessMultiplier = getLowEnd();
            brightnessMultiplier = pow(brightnessMultiplier + 0.8, 2.5);
            col += spectrum * spectrumColor * (brightnessMultiplier + freq * FREQUENCY_REACTIVENESS);
            col *= clamp(length(p) * 4.0, 0.0, 1.0);
            gl_FragColor = vec4(col * 0.5, 0.25);
        }