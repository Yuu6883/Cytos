export const makeProg = (gl: WebGLRenderingContext, vs_src: string, fs_src: string) => {
    const vs = gl.createShader(gl.VERTEX_SHADER);
    const fs = gl.createShader(gl.FRAGMENT_SHADER);

    gl.shaderSource(vs, vs_src);
    gl.shaderSource(fs, fs_src);

    gl.compileShader(vs);
    gl.compileShader(fs);

    const prog = gl.createProgram();
    gl.attachShader(prog, vs);
    gl.attachShader(prog, fs);
    gl.linkProgram(prog);

    if (!gl.getProgramParameter(prog, gl.LINK_STATUS)) {
        console.error(
            `vs info-log: ${gl.getShaderInfoLog(vs)}\n` +
                `info-log: ${gl.getShaderInfoLog(fs)}`,
        );
        throw new Error(`prog link failed: ${gl.getProgramInfoLog(prog)}`);
    }

    return prog;
};

export const shuffle = <T>(array: T[]) => {
    let currentIndex = array.length,
        randomIndex;

    // While there remain elements to shuffle...
    while (0 !== currentIndex) {
        // Pick a remaining element...
        randomIndex = Math.floor(Math.random() * currentIndex);
        currentIndex--;

        // And swap it with the current element.
        [array[currentIndex], array[randomIndex]] = [
            array[randomIndex],
            array[currentIndex],
        ];
    }

    return array;
};

export const PROTOCOL_VERSION = 'CYTOS 0.0.2';

export const GuestName = (hash: number) => `Guest#${hash.toString().padStart(4, '0')}`;

export const hexDump = (buffer: ArrayBuffer) =>
    [...new Uint8Array(buffer)].map(x => x.toString(16).padStart(2, '0')).join('');
