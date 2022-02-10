const FFT = 2048;

export class Visualizer {
    private gl: WebGL2RenderingContext;

    private audioCtx: AudioContext;

    private bigBOOM: AnalyserNode;
    private smolBOOM: AnalyserNode;

    private buffer1 = new Uint8Array(3 * (FFT >> 1));
    private buffer2 = new Uint8Array(FFT >> 1);

    private hasAudio = false;
    private tex: WebGLTexture;

    constructor(gl: WebGL2RenderingContext) {
        this.init(gl);
    }

    init(gl: WebGL2RenderingContext) {
        this.gl = gl;
        this.tex = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, this.tex);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
        gl.texImage2D(
            gl.TEXTURE_2D,
            0,
            gl.RGB,
            FFT >> 1,
            1,
            0,
            gl.RGB,
            gl.UNSIGNED_BYTE,
            null,
        );
    }

    get enabled() {
        return this.hasAudio;
    }

    get amp() {
        let sum = 0;
        for (let i = 250; i < 300; i++) sum += this.buffer2[i];
        return sum / 50 / 256;
    }

    update() {
        if (this.hasAudio) {
            this.smolBOOM.getByteFrequencyData(this.buffer1);
            this.bigBOOM.getByteFrequencyData(this.buffer2);

            for (let i = (FFT >> 1) - 1; i >= 0; i--) {
                this.buffer1[i * 3 + 0] = this.buffer1[i];
                this.buffer1[i * 3 + 1] = this.buffer1[i];
                this.buffer1[i * 3 + 2] = this.buffer1[i];
            }
            const gl = this.gl;

            gl.activeTexture(gl.TEXTURE0);
            gl.bindTexture(gl.TEXTURE_2D, this.tex);
            gl.texSubImage2D(
                gl.TEXTURE_2D,
                0,
                0,
                0,
                FFT >> 1,
                1,
                gl.RGB,
                gl.UNSIGNED_BYTE,
                this.buffer1,
            );
        }
    }

    stop() {
        this.hasAudio = false;
    }

    setStream(stream: MediaStream) {
        this.audioCtx = new AudioContext();
        this.bigBOOM = this.audioCtx.createAnalyser();
        this.bigBOOM.smoothingTimeConstant = 0.7;
        this.bigBOOM.fftSize = FFT;
        this.bigBOOM.minDecibels = -90;

        this.smolBOOM = this.audioCtx.createAnalyser();
        this.smolBOOM.smoothingTimeConstant = 0.9;
        this.smolBOOM.fftSize = FFT;

        const f = this.audioCtx.createBiquadFilter();
        f.type = 'allpass';

        const source = this.audioCtx.createMediaStreamSource(stream);
        source.connect(f);
        f.connect(this.bigBOOM);
        f.connect(this.smolBOOM);
        this.hasAudio = true;
    }

    prompt() {
        (navigator.mediaDevices as any)
            .getDisplayMedia({ video: true, audio: true })
            .then((stream: MediaStream) => this.setStream(stream));
    }
}
