export class AtlasTexture {
    static INVALID = new AtlasTexture(0, '', null, null, true);

    key: string;
    index: number;
    gl: WebGLRenderingContext;
    texture: Texture;
    uvs = new Float32Array(6);
    theme = new Float32Array([1, 1, 1]);
    valid = false;
    ref = 1;
    persistent: boolean;

    xOffset = 0;
    yOffset = 0;
    depth = 0;

    constructor(
        index: number,
        key: string,
        texture: Texture,
        gl: WebGLRenderingContext,
        p: boolean,
    ) {
        this.gl = gl;
        this.key = key;
        this.index = index;
        this.texture = texture;
        this.persistent = p;
    }

    get unit() {
        return this.texture.unit;
    }

    sub() {
        const nd = this.depth + 1;
        const texs = Array.from({ length: 4 }, _ => {
            const t = new AtlasTexture(
                this.index,
                '',
                this.texture,
                this.gl,
                this.persistent,
            );
            t.depth = nd;
            return t;
        });

        const { w, h, xBlock, yBlock } = this.texture;

        texs[1].xOffset = (w / xBlock) >>> nd;
        texs[2].yOffset = (h / yBlock) >>> nd;
        texs[3].xOffset = (w / xBlock) >>> nd;
        texs[3].yOffset = (h / yBlock) >>> nd;

        return texs;
    }

    buffer(
        source: ImageBitmap | HTMLCanvasElement,
        sw = source.width,
        sh = source.height,
    ) {
        if (!this.texture) return;

        const { w, h, xBlock, yBlock } = this.texture;

        sw = Math.min(sw, w / 2 ** this.depth);
        sh = Math.min(sh, h / 2 ** this.depth);

        const xOffset = Math.round(this.xOffset + (this.index % xBlock) * (w / xBlock));
        const yOffset = Math.round(this.yOffset + ~~(this.index / xBlock) * (h / yBlock));
        this.texture.bind();

        const gl = this.gl;
        gl.texSubImage2D(
            gl.TEXTURE_2D,
            0,
            xOffset,
            yOffset,
            gl.RGBA,
            gl.UNSIGNED_BYTE,
            source,
        );
        this.uvs.set([
            xOffset / w,
            yOffset / h,
            (xOffset + sw) / w,
            (yOffset + sh) / h,
            sw,
            sh,
        ]);
        this.valid = false;
        this.texture.mipmapSet.add(this);

        if (source instanceof ImageBitmap) source.close();
    }

    incre() {
        if (this.persistent || this.depth) return;
        this.ref++;
    }

    decre() {
        if (this.persistent || this.depth) return;
        this.ref--;
        if (this.ref <= 0) this.destroy();
    }

    private destroy() {
        this.valid = false;
        if (this.depth > 0) return;
        if (this.texture) this.texture.unusedIndices.push(this.index);
    }
}

class Texture {
    unit: number;
    private gl: WebGLRenderingContext;
    w: number;
    h: number;
    xBlock: number;
    yBlock: number;
    private tex: WebGLTexture;
    unusedIndices: number[] = [];
    mipmapSet: Set<AtlasTexture> = new Set();
    persistent: boolean;

    constructor(
        gl: WebGLRenderingContext,
        id: number,
        w: number,
        h: number,
        xBlock: number,
        yBlock: number,
        persistent: boolean,
    ) {
        this.gl = gl;
        this.w = w;
        this.h = h;
        this.xBlock = xBlock;
        this.yBlock = yBlock;
        this.unit = id;
        this.tex = gl.createTexture();
        this.persistent = persistent;

        this.bind();
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        gl.texImage2D(
            gl.TEXTURE_2D,
            0,
            gl.RGBA,
            w,
            h,
            0,
            gl.RGBA,
            gl.UNSIGNED_BYTE,
            null,
        );

        for (let i = 0; i < xBlock * yBlock; i++) this.unusedIndices.push(i);
        // console.log(`TEXTURE UNIT = ${this.unit}`);
    }

    get full() {
        return !this.unusedIndices.length;
    }
    get genMipmap() {
        return !!this.mipmapSet.size;
    }

    bind() {
        this.gl.activeTexture(this.gl.TEXTURE0 + this.unit);
        this.gl.bindTexture(this.gl.TEXTURE_2D, this.tex);
    }

    mipmap() {
        this.bind();
        this.gl.generateMipmap(this.gl.TEXTURE_2D);
        this.gl.texParameteri(
            this.gl.TEXTURE_2D,
            this.gl.TEXTURE_MIN_FILTER,
            this.gl.LINEAR_MIPMAP_LINEAR,
        );
        for (const a of this.mipmapSet) if (a.ref > 0) a.valid = true;
        this.mipmapSet.clear();
    }

    add(key: string) {
        const freeIndex = this.unusedIndices.shift();
        return new AtlasTexture(freeIndex, key, this, this.gl, this.persistent);
    }

    destroy() {
        this.gl.deleteTexture(this.tex);
    }
}

export class TextureStore {
    static EMPTY_TEX: WebGLTexture;

    gl: WebGLRenderingContext;
    w: number;
    h: number;
    xBlock: number;
    yBlock: number;
    maxStack: number;
    stack: Texture[] = [];
    startID: number;
    store: Map<string, AtlasTexture> = new Map();
    persistent: boolean;

    private temp = document.createElement('canvas');
    tempCtx = this.temp.getContext('2d');

    constructor(
        gl: WebGLRenderingContext,
        w: number,
        h: number,
        xBlock: number,
        yBlock: number,
        maxStack: number,
        startID: number,
        persistent = false,
    ) {
        this.w = w;
        this.h = h;
        this.xBlock = xBlock;
        this.yBlock = yBlock;
        this.maxStack = maxStack;
        this.startID = startID;

        this.temp.width = ~~(w / xBlock);
        this.temp.height = ~~(h / yBlock);
        this.persistent = persistent;
        this.init(gl);
    }

    public init(gl: WebGLRenderingContext) {
        this.gl = gl;
        this.tempCtx.clearRect(0, 0, this.temp.width, this.temp.height);
        this.stack.splice(0, this.stack.length);
        this.store.clear();

        if (!TextureStore.EMPTY_TEX) {
            TextureStore.EMPTY_TEX = gl.createTexture();
            gl.bindTexture(gl.TEXTURE_2D, TextureStore.EMPTY_TEX);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
            gl.texImage2D(
                gl.TEXTURE_2D,
                0,
                gl.RGBA,
                1,
                1,
                0,
                gl.RGBA,
                gl.UNSIGNED_BYTE,
                new Uint8Array([1, 1, 1, 1]),
            );

            for (let i = 0; i < 16; i++) {
                gl.activeTexture(gl.TEXTURE0 + i);
                gl.bindTexture(gl.TEXTURE_2D, TextureStore.EMPTY_TEX);
            }
        }
    }

    private save(key: string) {
        let t = this.stack.find(tex => !tex.full);
        if (!t && this.stack.length >= this.maxStack) return AtlasTexture.INVALID;
        if (!t) {
            t = new Texture(
                this.gl,
                this.stack.length + this.startID,
                this.w,
                this.h,
                this.xBlock,
                this.yBlock,
                this.persistent,
            );
            this.stack.push(t);
        }
        return t.add(key);
    }

    // VRAM usage by this store
    get usage() {
        return this.w * this.h * 4 * this.stack.length;
    }

    add(
        key = '',
        cb?: (
            ctx: CanvasRenderingContext2D,
            w: number,
            h: number,
        ) => Promise<{
            success: boolean;
            sw?: number;
            sh?: number;
            bitmap?: ImageBitmap;
        }>,
    ) {
        if (!key) return AtlasTexture.INVALID;
        if (this.store.has(key)) {
            this.store.get(key).incre();
            return this.store.get(key);
        } else {
            const t = this.save(key);
            this.store.set(key, t);
            if (cb)
                cb(this.tempCtx, this.temp.width, this.temp.height).then(result => {
                    if (!result.success) {
                        t.decre();
                        if (t.ref <= 0) this.store.delete(key);
                        return;
                    }
                    t.buffer(result.bitmap || this.temp, result.sw, result.sh);
                });
            return t;
        }
    }

    addSync(
        key = '',
        cb: (
            ctx: CanvasRenderingContext2D,
            w: number,
            h: number,
        ) => {
            success: boolean;
            sx?: number;
            sy?: number;
            sw?: number;
            sh?: number;
            bitmap?: ImageBitmap;
        },
    ) {
        if (!key) return AtlasTexture.INVALID;

        const exist = this.store.get(key);
        if (exist) {
            exist.incre();
            return exist;
        } else {
            const t = this.save(key);
            this.store.set(key, t);
            this.tempCtx.save();
            const result = cb(this.tempCtx, this.temp.width, this.temp.height);
            this.tempCtx.restore();
            if (!result.success) {
                t.decre();
                if (t.ref <= 0) this.store.delete(key);
                return AtlasTexture.INVALID;
            }
            t.buffer(result.bitmap || this.temp, result.sw, result.sh);
            return t;
        }
    }

    remove(key = '') {
        if (!key) return;
        if (this.store.has(key)) {
            const t = this.store.get(key);
            t.decre();
            if (t.ref <= 0) {
                this.store.delete(key);
                // console.log(`Key is removed from store: ${key}`);
            }
        }
    }

    genMipmap(limit = this.stack.length) {
        while (limit--) {
            const item = this.stack.find(t => t.genMipmap);
            if (item) item.mipmap();
            else return false;
        }
        return true;
    }

    clear() {
        for (const t of this.store.values()) t.valid = false;
        for (const t of this.stack) t.destroy();
        this.store.clear();
        this.stack.splice(0, this.stack.length);
    }

    rebind() {
        for (const t of this.stack) t.bind();
    }
}
