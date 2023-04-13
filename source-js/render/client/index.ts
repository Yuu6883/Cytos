import WebFont from 'webfontloader';
import { vec3, mat4 } from 'gl-matrix';

// Garbage React stuff
import { getResolution, Settings } from './settings/settings';
import { Themes } from './settings/themes';
import { HotkeyHandler } from './settings/hotkey-handler';
import { HUDStore, IStats, prettyBytes } from '../stores/hud';

import { rgbToHsl, hexToRGB, hslToRgb, getMainColor } from './cell/colors';

// URL's
import VirusSrc from '../img/virus.webp';
import BotSrc1 from '../img/bot1.webp';
import BotSrc2 from '../img/bot2.webp';
import BotSrc3 from '../img/bot3.webp';
import BotSrc4 from '../img/bot4.webp';
import EXPSrc1 from '../img/exp1.webp';
import EXPSrc2 from '../img/exp2.webp';
import EXPSrc3 from '../img/exp3.webp';
import EXPSrc4 from '../img/exp4.webp';
import CYTSrc1 from '../img/cyt1.webp';
import CYTSrc2 from '../img/cyt2.webp';
import CYTSrc3 from '../img/cyt3.webp';
import CYTSrc4 from '../img/cyt4.webp';
import RockSrc from '../img/rock.png';
import BASrc from '../img/ba.mp4';
// import IndicatorSrc from '../img/indicator.webp';

// Shaders
import SpriteVert from './shaders/sprite_vert.glsl';
import SpriteFrag from './shaders/sprite_frag.glsl';
import SimpleVert from './shaders/simple_vert.glsl';
import TrollFrag from './shaders/troll_frag.glsl';
import MapFrag from './shaders/map_frag.glsl';
import QuadVert from './shaders/quad_vert.glsl';
import DiscoFrag from './shaders/disco_frag.glsl';
import { makeProg } from './util';
import { AtlasTexture, TextureStore } from './cell/textures';
import { BasePlayer, Bot, Player } from './cell/player';

import { Visualizer } from './shaders/visualizer';
import { basicPopup } from '../react/components/panels/menu/views/popups';
import { Hotkeys } from './settings/keybinds';
import { SYS } from './util/sys_message';
import { GetInputs } from '../stores/inputs';
import { CytosTimings, CytosVersion, CytosInputData } from '../types';
import { CurrServer } from '../stores/servers';

const CIRCLE_RADIUS = 512;
const CIRCLE_PADDING = 6;
const COLOR_SAMPLE_SIZE = 64;

const QUAD_VERT = [-1, -1, +1, -1, -1, +1, +1, -1, -1, +1, +1, +1];

interface RenderModuleAddon {
    postInit(client: Client): void;
    render(client: Client, lerp: number, dt: number, debug: boolean): number;
    renderV(client: Client, lerp: number, dt: number, modifier: number): number;
    parse(client: Client, buf: ArrayBuffer): number;
    getCellColor(index: number): [number, number, number];
    clear(): void;
    getPID(x: number, y: number): number;
}

const RenderModule: RenderModuleAddon = eval("require('./gfx-addon.node')");

class InputHandler {
    private readonly client: Client;
    constructor(client: Client) {
        this.client = client;
    }

    public macro = false;
    public spectateTarget = 0;

    private readonly d_line = [false, false];
    private readonly d_macro = [false, false];
    private readonly d_spawn = [false, false];
    private readonly d_splits = new Uint32Array(2);
    private readonly d_ejects = new Uint8Array(2);
    private readonly d_cursors = [new Int32Array(2), new Int32Array(2)];
    private readonly d_cursor_unlock_timer = [0, 0];
    private readonly line_unlock_timer = [0, 0];
    private minion_abuz = 0;

    public update() {
        const active = this.client.hotkeys.minionMode.v
            ? (this.minion_abuz = 1 - this.minion_abuz)
            : this.tab;

        const input: CytosInputData = {
            activeTab: active,
            spectate: this.spectateTarget,
            data: [null, null],
        };

        const m = this.client.mouseOutTimestamp;
        const c = this.client.cursor.position;
        const live = !m || Date.now() - m < 1000;
        const v = this.client.vports;

        if (this.client.hotkeys.minionMode.v) {
            if (Date.now() > this.d_cursor_unlock_timer[0]) {
                this.d_cursors[0].set(live ? c.slice(0, 2) : [v[0], v[1]]);
            }
            if (Date.now() > this.d_cursor_unlock_timer[1]) {
                this.d_cursors[1].set(live ? c.slice(0, 2) : [v[2], v[3]]);
            }
        } else {
            // Otherwise only sync active tab
            const t = this.tab;
            if (Date.now() > this.d_cursor_unlock_timer[this.tab]) {
                const t = this.tab;
                this.d_cursors[t].set(
                    live ? c.slice(0, 2) : [v[t * 2 + 0], v[t * 2 + 1]],
                );
            }
        }
        this.d_macro[this.tab] = this.macro;

        for (let tab = 0; tab <= 1; tab++) {
            // Tab is line locked
            if ((this.client.flags[tab] >> 1) & 0x3) {
                const t = this.line_unlock_timer[tab];
                // Auto line unlock timer expired
                if (t && Date.now() >= t) {
                    this.d_line[tab] = true;
                }
            } else {
                this.line_unlock_timer[tab] = 0;
            }

            input.data[tab] = {
                line: this.d_line[tab],
                macro: this.d_macro[tab],
                spawn: this.d_spawn[tab],
                splits: this.d_splits[tab],
                ejects: this.d_ejects[tab],
                mouseX: this.d_cursors[tab][0],
                mouseY: this.d_cursors[tab][1],
            };
        }

        this.client.worker.postMessage({ input });
        this.d_line.fill(false);
        this.d_spawn.fill(false);
        this.d_splits.fill(0);
        this.d_ejects.fill(0);
        this.spectateTarget = 0;
    }

    private get tab() {
        return this.client.activeTab;
    }

    public get isDual() {
        return !!this.client.pids[1];
    }

    set spawn(value: boolean) {
        this.d_spawn[this.tab] = value;
    }

    set splits(value: number) {
        if (
            this.client.hotkeys.autoUnlockLine.v &&
            this.client.hotkeys.autoUnlockLineTimeout.v
        ) {
            this.line_unlock_timer[this.tab] =
                Date.now() + this.client.hotkeys.autoUnlockLineTimeout.v;
        }
        this.d_splits[this.tab] = value;
        this.sync_cursor();
    }

    set ejects(value: number) {
        this.d_ejects[this.tab] = value;
    }

    get splits() {
        return this.d_splits[this.tab];
    }

    get ejects() {
        return this.d_ejects[this.tab];
    }

    set line(value: boolean) {
        this.d_line[this.tab] = value;
    }

    private sync_cursor() {
        const m = this.client.mouseOutTimestamp;
        const c = this.client.cursor.position;
        const live = !m || Date.now() - m < 1000;
        const t = this.tab;
        this.d_cursors[t].set(
            live
                ? c.slice(0, 2)
                : [this.client.vports[t * 2 + 0], this.client.vports[t * 2 + 1]],
        );
    }

    private sync_cursor_minion() {
        const m = this.client.mouseOutTimestamp;
        const c = this.client.cursor.position;
        const live = !m || Date.now() - m < 1000;
        const v = this.client.vports;

        this.d_cursors[0].set(live ? c.slice(0, 2) : [v[0], v[1]]);
        this.d_cursors[1].set(live ? c.slice(0, 2) : [v[2], v[3]]);
    }

    new_line(value: number) {
        this.splits += value;
        this.d_cursor_unlock_timer[this.tab] =
            Date.now() + this.client.hotkeys.autoUnlockLineTimeout.v;
    }

    new_line_minion(value: number) {
        this.minion_splits += value;
        this.d_cursor_unlock_timer[1 - this.tab] =
            Date.now() + this.client.hotkeys.autoUnlockLineTimeout.v;
    }

    get minion_macro() {
        return this.d_macro[1 - this.tab];
    }

    get minion_splits() {
        return this.d_splits[1 - this.tab];
    }

    get minion_ejects() {
        return this.d_ejects[1 - this.tab];
    }

    get minion_line() {
        return this.d_line[1 - this.tab];
    }

    set minion_macro(value: boolean) {
        this.d_macro[1 - this.tab] = value;
    }

    set minion_splits(value: number) {
        this.d_splits[1 - this.tab] = value;
        this.sync_cursor_minion();
    }

    set minion_ejects(value: number) {
        this.d_ejects[1 - this.tab] = value;
    }

    set minion_line(value: boolean) {
        this.d_line[1 - this.tab] = value;
    }
}

export default class Client {
    public static instance: Client = null;
    public readonly input = new InputHandler(this);

    raf = 0;

    loaded = false;
    readonly canvas = document.createElement('canvas');
    static readonly sampler = document.createElement('canvas');
    static readonly samplerCtx = Client.sampler.getContext('2d');

    gl1: WebGLRenderingContext;
    gl2: WebGL2RenderingContext;
    vaoExt: OES_vertex_array_object;
    loseCtx: WEBGL_lose_context;

    quadVAO: WebGLVertexArrayObjectOES;
    spritesVAO: WebGLVertexArrayObjectOES;

    readonly fbo: Map<string, WebGLFramebuffer[]> = new Map();
    readonly buffers: Map<string, WebGLBuffer> = new Map();
    uniforms: WeakMap<WebGLProgram, Map<string, WebGLUniformLocation>>;

    readonly mouse = { x: 0, y: 0, scroll: 0 };
    readonly viewport = { w: 0, h: 0 };
    readonly viewbox = { t: 0, b: 0, l: 0, r: 0 };

    readonly cursor = { position: vec3.create() };
    readonly target = { position: vec3.create(), scale0: 10, scale1: 10 };
    readonly camera = { position: vec3.create(), scale0: 10, scale1: 10, tp: false };
    readonly proj = mat4.create();
    readonly screen = mat4.create();

    readonly playerData: Map<number, BasePlayer> = new Map();
    readonly myPlayerData: [Player, Player] = [null, null];

    readonly settings = Settings;
    readonly hotkeys = Hotkeys;
    readonly themes = Themes;

    readonly themeComputed = {
        activeColor: new Float32Array(3),
        inactiveColor: new Float32Array(3),
        foodColor: new Float32Array(3),
        ejectColor: new Float32Array(3),
    };

    mouseOutTimestamp = 0;
    fps = 0;
    readonly stats: IStats = {
        line: 0,
        cells: 0,
        fps: 0,
        score: 0,
    };

    circleTex: AtlasTexture;
    virusTex: AtlasTexture;
    ringTex: AtlasTexture;

    botTex: AtlasTexture[];
    expTex: AtlasTexture[];
    cytTex: AtlasTexture[];
    rockTex: AtlasTexture;

    activeTab = 0;
    spectating = true;

    readonly flags = new Uint8Array(2);
    readonly pids = new Int16Array(2);
    readonly cellCts = new Uint16Array(2);
    readonly vports = new Float32Array(4);
    readonly scores = new Float32Array(2);
    readonly map = new Float32Array(2);

    upsince = 0;
    lastRAF = 0;
    lastPacket = 0;

    rendercells = 0;
    bytesReceived = 0;
    gpuBytesUploaded = 0;

    visualizer: Visualizer;

    debug = false;
    readonly debugOutput = {};

    mapProg: WebGLProgram;
    spriteProg: WebGLProgram;
    vidProg: WebGLProgram;
    discoProg: WebGLProgram;
    readonly spriteBuffer = new Float32Array(65536 * 128); // 16MB vertex data

    skinStore: TextureStore;
    nameStore: TextureStore;
    massStore: TextureStore;
    readonly massWidth = new Float32Array(256);
    readonly charTex: Map<number, AtlasTexture> = new Map();

    primStore: TextureStore;
    vidTex: WebGLTexture;
    vidElem: HTMLVideoElement;

    readonly worker = new Worker(new URL('../worker.ts', import.meta.url));

    private static loadPromise: Promise<void>;

    public get gl() {
        return this.gl1 || this.gl2;
    }

    static init() {
        if (Client.instance) return Promise.resolve();
        if (Client.loadPromise) return Client.loadPromise;

        HotkeyHandler.init();

        return (Client.loadPromise = new Promise((resolve, reject) => {
            WebFont.load({
                google: {
                    families: ['Roboto'],
                },
                events: true,
                active: async () => {
                    const c = new Client();
                    try {
                        await c.init();
                        c.start();
                    } catch (e) {
                        console.error('OOGA BOOGA');
                        return reject(e);
                    }

                    Client.instance = c;
                    resolve();
                },
            });
        }));
    }

    private constructor() {
        this.viewport.w = this.canvas.width = window.innerWidth;
        this.viewport.h = this.canvas.height = window.innerHeight;

        this.canvas.id = 'game';
        document.body.appendChild(this.canvas);

        Client.sampler.width = Client.sampler.height = COLOR_SAMPLE_SIZE;

        this.resize = this.resize.bind(this);
        window.addEventListener('resize', this.resize);

        window.addEventListener(
            'wheel',
            (e: WheelEvent) => {
                const t = e.target as HTMLElement;
                if (
                    t === this.canvas ||
                    (t.getAttribute &&
                        t.getAttribute('panel-transition')?.includes('out'))
                )
                    this.mouse.scroll += e.deltaY;
            },
            { passive: true },
        );

        this.canvas.addEventListener('mousemove', (e: MouseEvent) => {
            this.mouseOutTimestamp = 0;
            let dx = (2 * e.clientX) / window.innerWidth - 1;
            let dy = (2 * e.clientY) / window.innerHeight - 1;
            const r = getResolution();
            const ratio = window.innerWidth / window.innerHeight / (r[0] / r[1]);
            if (ratio > 1) {
                dy /= ratio;
            } else {
                dx *= ratio;
            }
            this.mouse.x = dx;
            this.mouse.y = dy;
        });

        this.canvas.addEventListener('focus', () => HUDStore.chatInput.set(false));
        this.canvas.setAttribute('tabindex', '0');
        this.canvas.focus();

        document.addEventListener(
            'mouseleave',
            () => (this.mouseOutTimestamp = Date.now()),
        );
        document.addEventListener('mouseenter', () => (this.mouseOutTimestamp = 0));

        this.canvas.addEventListener(
            'webglcontextlost',
            e => {
                SYS.webglContextLost();
                e.preventDefault();
                setTimeout(() => this.loseCtx.restoreContext(), 500);
            },
            false,
        );

        this.canvas.addEventListener(
            'webglcontextrestored',
            _ => {
                this.init(true);
            },
            false,
        );

        type WorkerData = {
            mode?: string;
            event?: string;
            id?: string;
            pid0?: number;
            pid1?: number;
            rock?: number;
            timings?: CytosTimings;
            version?: CytosVersion;
            save?: string;
            bytes?: number;
            restore?: string;
        };

        // Handle "server" message
        this.worker.addEventListener(
            'message',
            (e: MessageEvent<Uint8Array | WorkerData>) => {
                const { data } = e;

                if (data instanceof Uint8Array) {
                    this.lastPacket = this.lastRAF;
                    this.bytesReceived += data.byteLength;

                    const oldMap = this.map.slice(0);
                    RenderModule.parse(this, data.buffer);

                    if (oldMap[0] !== this.map[0] || oldMap[1] !== this.map[1]) {
                        this.updateMapSize(this.map[0], this.map[1]);
                        HUDStore.nerdStats.map.set([this.map[0] * 2, this.map[1] * 2]);
                    }
                    this.postParse();
                    if (document.hidden) {
                        const now = performance.now();
                        const dt = now - this.lastRAF;
                        this.lastRAF = now;
                        RenderModule.render(this, 1, dt, false);
                    }
                } else if (typeof data === 'object') {
                    const {
                        mode,
                        event,
                        id,
                        pid0,
                        pid1,
                        timings,
                        version,
                        save,
                        bytes,
                        restore,
                    } = data;

                    if (mode !== undefined) {
                        if (mode === null) this.clear(false);
                    }
                    if (event === 'join') {
                        if (id === 'bot') {
                            this.playerData.set(pid0, new Bot(pid0));
                        } else if (id === 'player') {
                            // Keep cache cuz it's gonna be overwritten anyways
                            this.clear(true);

                            const [name, skin1, skin2] = GetInputs();
                            if (pid0) {
                                if (!this.myPlayerData[0])
                                    this.playerData.set(
                                        pid0,
                                        (this.myPlayerData[0] = new Player(
                                            pid0,
                                            name,
                                            skin1,
                                            '#FFFFFF',
                                        )),
                                    );
                            }
                            if (pid1) {
                                if (!this.myPlayerData[1])
                                    this.playerData.set(
                                        pid1,
                                        (this.myPlayerData[1] = new Player(
                                            pid1,
                                            name,
                                            skin2,
                                            '#FFFFFF',
                                        )),
                                    );
                            }
                        }
                    }
                    // Could be null
                    if (timings !== undefined) HUDStore.nerdStats.timings.set(timings);
                    if (version) {
                        this.upsince = version.timestamp;
                        HUDStore.nerdStats.version.set(version.version);
                        HUDStore.nerdStats.uptime.set(Date.now() - version.timestamp);
                        HUDStore.nerdStats.compile.set(version.compile);
                    }
                    if (save) SYS.msg(`Saved server: ${save} (${prettyBytes(bytes)})`);
                    if (restore)
                        SYS.msg(`Restored server: ${restore} (${prettyBytes(bytes)})`);
                } else console.error(`Received unexpected data from worker: `, data);
            },
        );

        setInterval(() => {
            this.stats.fps = this.fps;
            this.fps = 0;
        }, 1000);

        setInterval(() => {
            this.input.update();
        }, 1000 / 50);
    }

    public resize() {
        const DPR = window.devicePixelRatio;
        this.viewport.w = this.canvas.clientWidth * DPR;
        const r = getResolution();
        const ratio = window.innerWidth / window.innerHeight / (r[0] / r[1]);
        if (ratio < 1) {
            this.viewport.h = ratio * this.canvas.clientHeight * DPR;
        } else {
            this.viewport.h = this.canvas.clientHeight * DPR;
        }
    }

    public connect(mode: string) {
        this.worker.postMessage({ mode });
    }

    public save() {
        this.worker.postMessage({ save: true });
    }

    public restore(restore = CurrServer.mode.value) {
        if (restore) this.worker.postMessage({ restore });
    }

    public start() {
        if (this.raf) return false;
        const loop = now => {
            this.raf = requestAnimationFrame(loop);
            try {
                if (!this.gl.isContextLost()) this.render(now);
            } catch (e) {
                console.error(e);
                this.stop();
                this.connect(null);
                SYS.renderError();
            }
        };
        this.raf = requestAnimationFrame(loop);
        return true;
    }

    public stop() {
        if (!this.raf) return false;
        cancelAnimationFrame(this.raf);
        this.raf = 0;
        return true;
    }

    public render(now: number) {
        if (!this.lastRAF) {
            this.lastRAF = now;
            return;
        }

        this.fps++;
        this.resize();

        const dt = now - this.lastRAF;
        this.lastRAF = now;

        // Precompute value before passing into c++
        hexToRGB(this.themes.activeTabBorder.v, this.themeComputed.activeColor);
        hexToRGB(this.themes.inactiveTabBorder.v, this.themeComputed.inactiveColor);
        hexToRGB(this.themes.foodColor.v, this.themeComputed.foodColor);
        hexToRGB(this.themes.ejectColor.v, this.themeComputed.ejectColor);

        const lerp = Math.min(
            this.lastPacket
                ? (now - this.lastPacket) / (this.settings.drawDelay.v || 100)
                : 0,
            1,
        );

        this.updateTarget();
        this.camera.tp
            ? this.teleportCamera()
            : this.updateCamera(dt / this.settings.drawDelay.v);
        this.checkResolution();
        this.renderMap();

        const gl = this.gl;

        if (this.visualizer?.enabled && this.vidElem) {
            gl.useProgram(this.vidProg);
            this.bindVAO(this.spritesVAO);
            gl.uniformMatrix4fv(
                this.getUniform(this.vidProg, 'p'),
                false,
                this.proj as Float32Array,
            );

            if (this.vidElem.readyState >= 2) {
                gl.activeTexture(gl.TEXTURE0);
                gl.bindTexture(gl.TEXTURE_2D, this.vidTex);
                gl.texSubImage2D(
                    gl.TEXTURE_2D,
                    0,
                    0,
                    0,
                    gl.RGBA,
                    gl.UNSIGNED_BYTE,
                    this.vidElem,
                );
            }

            const modifier = Math.max(0.96, 1 + 0.5 * (this.visualizer.amp - 0.1));

            const len = RenderModule.renderV(this, lerp, dt, modifier);

            const glBuffer = this.buffers.get('s');
            const sub = this.spriteBuffer.subarray(0, len);

            gl.bindBuffer(gl.ARRAY_BUFFER, glBuffer);
            gl.bufferSubData(gl.ARRAY_BUFFER, 0, sub);
            gl.drawArrays(this.gl.TRIANGLES, 0, len / 9);

            this.gpuBytesUploaded +=
                sub.byteLength + this.vidElem.videoWidth * this.vidElem.videoHeight * 4;
        } else {
            gl.useProgram(this.spriteProg);
            this.bindVAO(this.spritesVAO);
            gl.uniformMatrix4fv(
                this.getUniform(this.spriteProg, 'p'),
                false,
                this.proj as Float32Array,
            );

            if ((window as any).debug > 0) {
                (window as any).debug--;
                this.debug = true;
            }

            const len = RenderModule.render(this, lerp, dt, this.debug);
            if (this.debug) {
                this.debug = false;
                console.log(this.debugOutput);
            }

            const glBuffer = this.buffers.get('s');
            const sub = this.spriteBuffer.subarray(0, len);

            gl.bindBuffer(gl.ARRAY_BUFFER, glBuffer);
            gl.bufferSubData(gl.ARRAY_BUFFER, 0, sub);
            gl.drawArrays(this.gl.TRIANGLES, 0, len / 9);

            this.gpuBytesUploaded += sub.byteLength;
        }
    }

    public checkResolution() {
        const gl = this.gl;
        const r = getResolution();
        if (gl.canvas.width != r[0] || gl.canvas.height != r[1]) {
            gl.canvas.width = r[0];
            gl.canvas.height = r[1];
            mat4.ortho(this.screen, 0, r[0], r[1], 0, 0, 1);
        }
    }

    private teleportCamera() {
        this.camera.scale0 = this.target.scale0;
        this.camera.scale1 = this.target.scale1;
        (this.camera.position as Float32Array).set(this.target.position);
        this.camera.tp = false;
    }

    public tpSeparateViewport() {
        const c = this.camera.position;
        const t = this.target.position;
        c[0] = t[0] = this.vports[this.activeTab * 2 + 0];
        c[1] = t[1] = this.vports[this.activeTab * 2 + 1];
    }

    private screenToWorld(out = vec3.create(), x = 0, y = 0) {
        const temp = mat4.create();
        mat4.invert(temp, this.proj);
        vec3.transformMat4(out, [x, -y, 0], temp);
    }

    private updateTarget() {
        this.screenToWorld(this.cursor.position, this.mouse.x, this.mouse.y);
        const scroll = this.mouse.scroll;
        this.mouse.scroll = 0;

        if (this.settings.cameraMode.v) {
            if (!this.activeTab) {
                this.target.scale0 *= 1 + scroll * 0.001;
                this.target.scale0 = Math.min(Math.max(this.target.scale0, 0.01), 10000);
            } else {
                this.target.scale1 *= 1 + scroll * 0.001;
                this.target.scale1 = Math.min(Math.max(this.target.scale1, 0.01), 10000);
            }
        } else {
            this.target.scale0 *= 1 + scroll * 0.001;
            this.target.scale1 = this.target.scale0 = Math.min(
                Math.max(this.target.scale0, 0.01),
                10000,
            );
        }
    }

    private updateCamera(d = 1 / 60) {
        const l = Math.min(Math.max(d * this.settings.cameraSpeed.v, 0), 1);
        vec3.lerp(
            this.camera.position,
            this.camera.position,
            this.target.position,
            l / 3,
        );
        this.camera.scale0 += (this.target.scale0 - this.camera.scale0) * l;
        this.camera.scale1 += (this.target.scale1 - this.camera.scale1) * l;

        const x = this.camera.position[0];
        const y = this.camera.position[1];
        const scale = this.activeTab ? this.camera.scale1 : this.camera.scale0;

        const hw = (this.viewport.w * scale) / 2;
        const hh = (this.viewport.h * scale) / 2;

        const v = this.viewbox;
        mat4.ortho(
            this.proj,
            (v.l = x - hw),
            (v.r = x + hw),
            (v.b = y - hh),
            (v.t = y + hh),
            0,
            1,
        );
    }

    public spectateAtCursor(biggest = false) {
        this.input.spectateTarget = biggest ? 65535 : this.pidAtCursor();
    }

    public spectate(pid: number) {
        this.input.spectateTarget = pid;
    }

    private pidAtCursor() {
        return RenderModule.getPID(this.cursor.position[0], this.cursor.position[1]);
    }

    public getUserAtCursor() {
        const pid = this.pidAtCursor();
        if (!pid) return { empty: true };
        if (pid === 16380) return { cyt: true };
        if (pid === 16379) return { exp: true };
        const player = this.playerData.get(pid);
        if (!player) return { unknown: true, pid: pid };
        if (player.isBot) return { bot: true, pid: pid };
        const p = player as Player;
        return {
            pid: pid,
            skinState: p.skinState,
        };
    }

    get isAlive() {
        return [this.flags[0] & 1, this.flags[1] & 1];
    }

    public continueOrRespawn() {
        if (this.spectating || (!this.isAlive[0] && !this.isAlive[1]))
            this.input.spawn = true;
    }

    private createVAO() {
        return this.gl2
            ? this.gl2.createVertexArray()
            : this.vaoExt.createVertexArrayOES();
    }

    private bindVAO(vao: WebGLVertexArrayObject) {
        this.gl2 ? this.gl2.bindVertexArray(vao) : this.vaoExt.bindVertexArrayOES(vao);
    }

    private allocBuffer(name: string) {
        const buf = this.gl.createBuffer();
        this.buffers.set(name, buf);
        return buf;
    }

    private allocFrameBuffers(name: string, length = 1) {
        const bufs = Array.from({ length }, _ => this.gl.createFramebuffer());
        this.fbo.set(name, bufs);
        return bufs;
    }

    private loadUniform(prog: WebGLProgram, ...names: string[]) {
        if (!this.uniforms.has(prog)) this.uniforms.set(prog, new Map());
        for (const n of names) {
            const loc = this.gl.getUniformLocation(prog, n);
            if (loc) this.uniforms.get(prog).set(n, loc);
        }
    }

    private getUniform(prog: WebGLProgram, name: string) {
        return this.uniforms.get(prog).get(name);
    }

    private async init(reinit = false) {
        if (!this.gl2)
            this.gl2 = this.canvas.getContext('webgl2', {
                alpha: false,
                antialias: false,
                depth: false,
                failIfMajorPerformanceCaveat: false,
                powerPreference: 'high-performance',
                premultipliedAlpha: false,
                preserveDrawingBuffer: true,
                stencil: false,
            });

        if (!this.gl2) {
            this.gl1 = this.canvas.getContext('webgl', {
                alpha: false,
                antialias: false,
                depth: false,
                failIfMajorPerformanceCaveat: false,
                powerPreference: 'high-performance',
                premultipliedAlpha: false,
                preserveDrawingBuffer: true,
                stencil: false,
            });
        }

        // POTATO DETECTED
        if (!this.gl) return void basicPopup('Your browser does not support WebGL1/2');
        if (this.gl1 && !this.vaoExt)
            this.vaoExt = this.gl1.getExtension('OES_vertex_array_object');
        if (!this.loseCtx) this.loseCtx = this.gl.getExtension('WEBGL_lose_context');
        if (this.gl2) {
            if (this.visualizer) this.visualizer.init(this.gl2);
            else this.visualizer = new Visualizer(this.gl2);
        }

        const gl = this.gl;

        gl.enable(gl.BLEND);
        // gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
        gl.blendFuncSeparate(
            gl.SRC_ALPHA,
            gl.ONE_MINUS_SRC_ALPHA,
            gl.ONE,
            gl.ONE_MINUS_SRC_ALPHA,
        );
        // gl.blendFunc(gl.ONE, gl.ONE_MINUS_SRC_ALPHA);
        delete TextureStore.EMPTY_TEX;
        this.initTextureStores();

        this.fbo.clear();
        this.buffers.clear();
        this.uniforms = new WeakMap();

        this.prepMapProg(this.gl);
        this.prepDiscoProg(this.gl);
        this.prepVidProg(this.gl);
        this.prepSpriteProg(this.gl);

        this.initTextures();
        this.resize();

        if (reinit) {
            for (const [_, p] of this.playerData) {
                if (p.isBot) continue;
                this.updateNameTexture(p as Player);
                this.updateSkinTexture(p as Player);
            }

            this.updateMapSize(this.map[0], this.map[1]);
            return;
        }

        setInterval(
            () =>
                this.primStore.genMipmap(1) ||
                this.massStore.genMipmap(1) ||
                this.skinStore.genMipmap(1) ||
                this.nameStore.genMipmap(1),
            500,
        );

        setInterval(() => {
            HUDStore.nerdStats.batch(s => {
                s.uptime.set(Date.now() - this.upsince);
                s.rendercells.set(this.rendercells);
                s.bandwidth.set(this.bytesReceived);
                s.gpuBandwidth.set(this.gpuBytesUploaded);
            });

            this.bytesReceived = 0;
            this.gpuBytesUploaded = 0;
        }, 1000);

        this.vidElem = document.createElement('video');
        this.vidElem.style.display = 'none';
        this.vidElem.autoplay = true;
        this.vidElem.onplay = () => {
            const stream = (this.vidElem as any).captureStream() as MediaStream;
            this.visualizer.setStream(stream);
        };
        this.vidElem.onended = () => this.visualizer.stop();
        document.body.appendChild(this.vidElem);

        RenderModule.postInit(this);
    }

    private initTextureStores() {
        const gl = this.gl;
        if (this.skinStore) this.skinStore.init(gl);
        else this.skinStore = new TextureStore(gl, 4096, 4096, 4, 4, 10, 6); // unit 6 -

        if (this.nameStore) this.nameStore.init(gl);
        else this.nameStore = new TextureStore(gl, 4096, 4096, 4, 16, 3, 3); // unit 3 - 5

        if (this.massStore) this.massStore.init(gl);
        else this.massStore = new TextureStore(gl, 2048, 2048, 8, 8, 1, 2, true); // unit 2

        if (this.primStore) this.primStore.init(gl);
        else this.primStore = new TextureStore(gl, 4096, 4096, 4, 4, 1, 1, true); // unit 1
        // Texture 0 is not used, let's put it to good use :))))

        const nCtx = this.nameStore.tempCtx;
        nCtx.font = 'bold 100px Roboto';
        nCtx.textAlign = 'left';
        nCtx.lineWidth = 10;
        nCtx.textBaseline = 'top';

        const mCtx = this.massStore.tempCtx;
        mCtx.font = 'bold 200px Roboto';
        mCtx.fillStyle = 'white';
        mCtx.strokeStyle = 'black';
        mCtx.textAlign = 'left';
        mCtx.lineWidth = 20;
        mCtx.textBaseline = 'top';

        this.skinStore.tempCtx.imageSmoothingEnabled = true;
        this.skinStore.tempCtx.imageSmoothingQuality = 'high';
    }

    private async bitmapFromURL(url: string) {
        const res = await fetch(url);
        const buf = await res.blob();
        return await createImageBitmap(buf);
    }

    private initTextures() {
        this.botTex = [BotSrc1, BotSrc2, BotSrc3, BotSrc4].map((src, index) =>
            this.primStore.add(`b${index}`, async () => {
                const bitmap = await this.bitmapFromURL(src);
                return { success: true, bitmap: bitmap };
            }),
        );

        this.expTex = this.primStore.add('exp').sub();

        [EXPSrc1, EXPSrc2, EXPSrc3, EXPSrc4].map(async (src, index) => {
            const bitmap = await this.bitmapFromURL(src);
            this.expTex[index].buffer(bitmap);
        });

        this.cytTex = this.primStore.add('cyt').sub();

        [CYTSrc1, CYTSrc2, CYTSrc3, CYTSrc4].map(async (src, index) => {
            const bitmap = await this.bitmapFromURL(src);
            this.cytTex[index].buffer(bitmap);
        });

        this.rockTex = this.primStore.add('rock', async () => {
            const bitmap = await this.bitmapFromURL(RockSrc);
            return { success: true, bitmap: bitmap };
        });

        this.virusTex = this.primStore.add('virus', async () => {
            const bitmap = await this.bitmapFromURL(VirusSrc);
            return { success: true, bitmap: bitmap };
        });

        this.circleTex = this.primStore.addSync('circle', (ctx, w, h) => {
            const R = CIRCLE_RADIUS - CIRCLE_PADDING;

            ctx.clearRect(0, 0, w, h);
            ctx.fillStyle = 'white';
            ctx.beginPath();
            ctx.arc(CIRCLE_RADIUS, CIRCLE_RADIUS, R, 0, 2 * Math.PI, false);
            ctx.closePath();
            ctx.fill();

            return { success: true };
        });

        this.ringTex = this.primStore.addSync('ring', (ctx, w, h) => {
            const R = CIRCLE_RADIUS - CIRCLE_PADDING;
            ctx.clearRect(0, 0, w, h);
            ctx.fillStyle = 'white';
            const path = new Path2D();
            path.arc(CIRCLE_RADIUS, CIRCLE_RADIUS, R, 0, 2 * Math.PI, false);
            path.arc(CIRCLE_RADIUS, CIRCLE_RADIUS, R * 0.925, 0, 2 * Math.PI, false);
            ctx.fill(path, 'evenodd');

            return { success: true };
        });

        for (const c of '0123456789.KM'.split('')) {
            const t = this.massStore.addSync(c, (ctx, w, h) => {
                ctx.clearRect(0, 0, w, h);
                ctx.strokeText(c, 20, 20);
                ctx.fillText(c, 20, 20);

                const m = ctx.measureText(c);
                const sw =
                    Math.abs(m.actualBoundingBoxLeft) +
                    Math.abs(m.actualBoundingBoxRight) +
                    40;
                this.massWidth[c.charCodeAt(0)] = sw / w;
                return { success: true, sw };
            });
            this.charTex.set(c.charCodeAt(0), t);
        }

        const gl = this.gl;
        gl.activeTexture(gl.TEXTURE0);
        this.vidTex = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, this.vidTex);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        gl.texImage2D(
            gl.TEXTURE_2D,
            0,
            gl.RGBA,
            512,
            512,
            0,
            gl.RGBA,
            gl.UNSIGNED_BYTE,
            null,
        );
    }

    private prepSpriteProg(gl: WebGLRenderingContext) {
        const prog = (this.spriteProg = makeProg(gl, SpriteVert, SpriteFrag));
        gl.useProgram(prog);
        this.loadUniform(prog, 'p', 's');
        gl.uniform1iv(
            this.getUniform(prog, 's'),
            new Int32Array(Array.from({ length: 16 }, (_, i) => i)),
        );

        this.spritesVAO = this.createVAO();
        this.bindVAO(this.spritesVAO);

        const buffer = this.allocBuffer('s');
        gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
        gl.bufferData(gl.ARRAY_BUFFER, this.spriteBuffer, gl.DYNAMIC_DRAW);

        const loc1 = gl.getAttribLocation(prog, 'v');
        gl.enableVertexAttribArray(loc1);
        gl.vertexAttribPointer(loc1, 2, gl.FLOAT, false, 36, 0);

        const loc2 = gl.getAttribLocation(prog, 'u');
        gl.enableVertexAttribArray(loc2);
        gl.vertexAttribPointer(loc2, 2, gl.FLOAT, false, 36, 8);

        const loc3 = gl.getAttribLocation(prog, 'c');
        gl.enableVertexAttribArray(loc3);
        gl.vertexAttribPointer(loc3, 4, gl.FLOAT, false, 36, 16);

        const loc4 = gl.getAttribLocation(prog, 't');
        gl.enableVertexAttribArray(loc4);
        gl.vertexAttribPointer(loc4, 1, gl.FLOAT, false, 36, 32);
    }

    public playVid() {
        this.vidElem.src = BASrc;
    }

    private prepVidProg(gl: WebGLRenderingContext) {
        this.vidProg = makeProg(gl, SpriteVert, TrollFrag);
        gl.useProgram(this.vidProg);
        this.loadUniform(this.vidProg, 'p');
    }

    private prepMapProg(gl: WebGLRenderingContext) {
        const prog = (this.mapProg = makeProg(gl, QuadVert, MapFrag));
        gl.useProgram(prog);
        this.loadUniform(prog, 'p', 'b');

        this.quadVAO = this.createVAO();
        this.bindVAO(this.quadVAO);
        const buffer = this.allocBuffer('m');
        gl.bindBuffer(gl.ARRAY_BUFFER, buffer);

        const INIT_M = 10000;
        gl.bufferData(
            gl.ARRAY_BUFFER,
            new Float32Array(
                [
                    -INIT_M,
                    -INIT_M,
                    +INIT_M,
                    -INIT_M,
                    -INIT_M,
                    +INIT_M,
                    +INIT_M,
                    -INIT_M,
                    -INIT_M,
                    +INIT_M,
                    +INIT_M,
                    +INIT_M,
                ].concat(QUAD_VERT),
            ),
            gl.STATIC_DRAW,
        );

        const loc1 = gl.getAttribLocation(prog, 'a');
        gl.enableVertexAttribArray(loc1);
        gl.vertexAttribPointer(loc1, 2, gl.FLOAT, false, 0, 0);

        const loc2 = gl.getAttribLocation(prog, 'u');
        gl.enableVertexAttribArray(loc2);
        gl.vertexAttribPointer(loc2, 2, gl.FLOAT, false, 0, 48);
    }

    private prepDiscoProg(gl: WebGLRenderingContext) {
        const prog = (this.discoProg = makeProg(gl, QuadVert, DiscoFrag));
        gl.useProgram(prog);
        this.loadUniform(prog, 'p', 'tex', 'time', 'color');
    }

    public updateMapSize(w: number, h: number) {
        const gl = this.gl;
        gl.useProgram(this.mapProg);
        const buffer = this.buffers.get('m');
        gl.bindBuffer(gl.ARRAY_BUFFER, buffer);

        const W = w + 50;
        const H = h + 50;
        gl.bufferSubData(
            gl.ARRAY_BUFFER,
            0,
            new Float32Array([-W, -H, +W, -H, -W, +H, +W, -H, -W, +H, +W, +H]),
        );
    }

    clearCache() {
        this.playerData.clear();
        this.myPlayerData.fill(null);
        this.skinStore.clear();
        this.nameStore.clear();
    }

    clear(keepCache = false) {
        this.spriteBuffer.fill(0);
        RenderModule.clear();
        keepCache || this.clearCache();
        // this.protocol.lbmm.clear();
        this.stats.cells = 0;
        this.stats.line = 0;
        this.stats.score = 0;
    }

    public switchTab() {
        this.activeTab = 1 - this.activeTab;
        if (this.settings.cameraMode.v) this.tpSeparateViewport();
    }

    public filterLongString(name: string) {
        while (true) {
            const m = this.nameStore.tempCtx.measureText(name);
            const w =
                Math.abs(m.actualBoundingBoxLeft) + Math.abs(m.actualBoundingBoxRight);
            const h =
                Math.abs(m.actualBoundingBoxDescent) +
                Math.abs(m.actualBoundingBoxAscent);
            if (w < 1000 && h < 250) break;
            name = name.slice(0, -1);
        }
        return name;
    }

    public getPlayer(pid: number) {
        if (this.playerData.has(pid)) return this.playerData.get(pid);
        const p = new Bot(pid);
        this.playerData.set(pid, p);
        return p;
    }

    public updateNameTexture(player: Player) {
        if (!player.name) {
            player.nameTex = AtlasTexture.INVALID;
            return;
        }

        const [r, g, b] = hexToRGB(player.color).map(i => i * 255);
        const border = /border: (none|#[0-9a-f]{6})/i.exec(player.color)?.[1];
        const color = `rgb(${~~r},${~~g},${~~b})`;

        player.nameTex = this.nameStore.addSync(
            `${player.name}#${player.color}`,
            (ctx, w, h) => {
                const [_, __, l] = rgbToHsl(r, g, b);

                ctx.save();
                ctx.miterLimit = 2;
                ctx.fillStyle = color;

                if (l > 0.25) {
                    ctx.strokeStyle = 'black';
                } else {
                    ctx.strokeStyle = 'white';
                }

                const m = ctx.measureText(player.name);

                const sw =
                    Math.abs(m.actualBoundingBoxLeft) +
                    Math.abs(m.actualBoundingBoxRight) +
                    40;
                const sh =
                    Math.abs(m.actualBoundingBoxDescent) +
                    Math.abs(m.actualBoundingBoxAscent) +
                    40;

                if (border && border !== 'none') ctx.strokeStyle = border;

                ctx.clearRect(0, 0, w, h);
                if (border !== 'none') ctx.strokeText(player.name, 20, 20);

                ctx.fillText(player.name, 20, 20);
                ctx.restore();

                return { success: true, sw, sh };
            },
        );
    }

    public updateSkinState(pid: number, state: number) {
        const base = this.playerData.get(pid);
        if (base?.isBot) return;
        const player = base as Player;
        if (player.skinState < 0) return;
        player.skinState = state;
    }

    updateSkinTexture(player: Player) {
        if (!player.skin) {
            player.skinTex = AtlasTexture.INVALID;
            player.skinState = -1;
            return;
        }

        const rgb = RenderModule.getCellColor(player.pid).map(n => ~~(255 * n));
        const color = `rgb(${rgb.join(',')})`;

        const t = (player.skinTex = this.skinStore.add(
            `${player.skin}#${color}`,
            async (ctx, w, h) => {
                const skin = await this.bitmapFromURL(player.skin);
                if (!skin) {
                    player.skinState = -1;
                    return { success: false };
                } else player.skinState = 1;

                ctx.save();

                ctx.clearRect(0, 0, w, h);
                ctx.beginPath();
                ctx.arc(512, 512, 500, 0, 2 * Math.PI);
                ctx.clip();

                ctx.fillStyle = color;
                ctx.fill();
                ctx.restore();

                ctx.arc(512, 512, 506, 0, 2 * Math.PI);
                ctx.clip();
                ctx.drawImage(skin, 6, 6, 1018, 1018);
                ctx.restore();

                const sampler = await createImageBitmap(skin, {
                    resizeQuality: 'high',
                    resizeWidth: skin.width / 4,
                    resizeHeight: skin.height / 4,
                });
                const theme = Client.sampleColor(sampler, color);
                sampler.close();
                t.theme.set(theme);

                return { success: true };
            },
        ));
    }

    public postParse() {
        const t = this.target;

        if (this.hotkeys.minionMode.v) {
            const camTab = this.isAlive[this.activeTab]
                ? this.activeTab
                : 1 - this.activeTab;
            t.position[0] = this.vports[camTab * 2 + 0];
            t.position[1] = this.vports[camTab * 2 + 1];
        } else if (this.settings.cameraMode.v) {
            t.position[0] = this.vports[this.activeTab * 2 + 0];
            t.position[1] = this.vports[this.activeTab * 2 + 1];
        } else if (
            (!this.isAlive[0] && !this.isAlive[1]) ||
            (!this.scores[0] && !this.scores[1])
        ) {
            t.position[0] = this.vports[0];
            t.position[1] = this.vports[1];
        } else if (this.isAlive[0] && !this.isAlive[1]) {
            t.position[0] = this.vports[0];
            t.position[1] = this.vports[1];
        } else if (!this.isAlive[0] && this.isAlive[1]) {
            t.position[0] = this.vports[2];
            t.position[1] = this.vports[3];
        } else {
            t.position[0] = (this.vports[0] + this.vports[2]) / 2;
            t.position[1] = (this.vports[1] + this.vports[3]) / 2;
        }

        this.stats.line = (this.flags[this.activeTab] >> 1) & 0x3;
        this.stats.cells = this.cellCts[this.activeTab];
        this.stats.score = this.scores[0] + this.scores[1];
    }

    private renderMap() {
        const gl = this.gl;
        gl.viewport(0, 0, gl.drawingBufferWidth, gl.drawingBufferHeight);
        gl.bindFramebuffer(gl.FRAMEBUFFER, null);
        const rgb = hexToRGB(this.themes.background.v, new Float32Array([0, 0, 0]));
        gl.clearColor(rgb[0], rgb[1], rgb[2], 0);
        gl.clear(gl.COLOR_BUFFER_BIT);

        if (this.visualizer?.enabled) {
            this.visualizer.update();
            gl.useProgram(this.discoProg);
            gl.uniformMatrix4fv(
                this.getUniform(this.discoProg, 'p'),
                false,
                this.proj as Float32Array,
            );
            gl.uniform1f(this.getUniform(this.discoProg, 'time'), this.lastRAF * 0.001);
            const hue = (~~(this.lastRAF / 50) % 360) / 360;
            const [r, g, b] = hslToRgb(hue, 0.99, 0.5);
            gl.uniform3f(this.getUniform(this.discoProg, 'color'), r, g, b);
        } else {
            const mapColor = hexToRGB(
                this.themes.mapColor.v,
                new Float32Array([0.1875, 0.1875, 0.1875]),
            );
            gl.useProgram(this.mapProg);
            gl.uniform4f(
                this.getUniform(this.mapProg, 'b'),
                mapColor[0],
                mapColor[1],
                mapColor[2],
                1,
            );
            gl.uniformMatrix4fv(
                this.getUniform(this.mapProg, 'p'),
                false,
                this.proj as Float32Array,
            );
        }

        this.bindVAO(this.quadVAO);
        gl.drawArrays(gl.TRIANGLES, 0, 6);
    }

    public static sampleColor(img: ImageBitmap | HTMLImageElement, color: string) {
        const ctx = this.samplerCtx;
        ctx.save();

        ctx.clearRect(0, 0, COLOR_SAMPLE_SIZE, COLOR_SAMPLE_SIZE);
        ctx.beginPath();
        ctx.arc(
            COLOR_SAMPLE_SIZE >>> 1,
            COLOR_SAMPLE_SIZE >>> 1,
            COLOR_SAMPLE_SIZE >>> 1,
            0,
            2 * Math.PI,
        );
        ctx.clip();

        ctx.fillStyle = color;
        ctx.fill();
        ctx.drawImage(img, 0, 0, COLOR_SAMPLE_SIZE, COLOR_SAMPLE_SIZE);
        ctx.restore();

        return getMainColor(
            ctx.getImageData(0, 0, COLOR_SAMPLE_SIZE, COLOR_SAMPLE_SIZE).data,
        ) as [number, number, number];
    }
}
