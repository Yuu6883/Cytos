export const rgbToHsl = (r: number, g: number, b: number) => {
    r /= 255;
    g /= 255;
    b /= 255;

    const max = Math.max(r, g, b),
        min = Math.min(r, g, b);
    let h = 0,
        s = 0,
        l = (max + min) / 2;

    if (max !== min) {
        const d = max - min;
        s = l > 0.5 ? d / (2 - max - min) : d / (max + min);

        switch (max) {
            case r:
                h = (g - b) / d + (g < b ? 6 : 0);
                break;
            case g:
                h = (b - r) / d + 2;
                break;
            case b:
                h = (r - g) / d + 4;
                break;
        }
        h /= 6;
    }

    return [h, s, l];
};

const hue2rgb = (p: number, q: number, t: number) => {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1 / 6) return p + (q - p) * 6 * t;
    if (t < 1 / 2) return q;
    if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;
    return p;
};

export const hslToRgb = (h: number, s: number, l: number) => {
    let r = 0,
        g = 0,
        b = 0;

    if (!s) {
        r = g = b = l; // achromatic
    } else {
        var q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        var p = 2 * l - q;

        r = hue2rgb(p, q, h + 1 / 3);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1 / 3);
    }

    return [r, g, b] as [number, number, number];
};

export const hexToRGB = (hex: string, out = new Float32Array(3)) => {
    const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})/i.exec(hex);
    if (result) out.set(result.slice(1, 4).map(n => parseInt(n, 16) / 255));
    return out;
};

const HUE_BINS = 32;

export const getMainColor = (arr: Uint8ClampedArray) => {
    let i = 0;

    let pickIndex = 0;
    let hueMax = 0;
    const hw = new Float32Array(HUE_BINS);
    const ws = new Float32Array(HUE_BINS);
    const sa = new Float32Array(HUE_BINS);
    const li = new Float32Array(HUE_BINS);

    while (i < arr.length) {
        const [r, g, b, a] = arr.subarray(i, i + 4);
        i += 4; // skip alpha
        if (!a) continue;
        const [h, s, l] = rgbToHsl(r, g, b);
        if (l < 0.25 || l > 0.9) continue;
        const hueIndex = Math.round(h * HUE_BINS);

        const weight = s * (1 - Math.abs(l - 0.5));
        for (let j = -2; j <= 2; j++) {
            const w = weight - Math.abs(j) * 0.01;
            const index = (j + hueIndex + HUE_BINS) % HUE_BINS;
            const curr = (hw[index] += w);
            if (curr > hueMax) {
                pickIndex = index;
                hueMax = curr;
            }
        }

        ws[hueIndex] += weight;
        sa[hueIndex] += weight * s;
        li[hueIndex] += weight * l;
    }

    if (!hueMax) return [0.8, 0.8, 0.8];

    const h = pickIndex / HUE_BINS;
    let s = sa[pickIndex] / ws[pickIndex];
    const l = li[pickIndex] / ws[pickIndex];

    s += (1 - s) * 0.5;

    return hslToRgb(h, s, l) as [number, number, number];
};
