const setting = (v = true) => ({ v });
const tex = () => ({
    valid: true,
    uvs: new Float32Array([0, 0, 1, 1, 128, 128]),
    theme: new Float32Array([1, 1, 1]),
});
const arr4 = [0, 0, 0, 0];

const client = {
    lastRAF: 0,

    spriteBuffer: new Float32Array(65536 * 64),
    massWidth: new Float32Array(256),

    charTex: new Map(),
    playerData: new Map(),

    viewbox: { t: 0, b: 0, l: 0, r: 0 },

    spectating: true,

    ringTex: tex(),
    virusTex: tex(),
    circleTex: tex(),
    rockTex: tex(),
    botTex: arr4.map(tex),
    expTex: arr4.map(tex),
    cytTex: arr4.map(tex),

    settings: {
        renderFood: setting(),
        renderSkin: setting(),
        renderName: setting(),
        renderMass: setting(2),
    },

    themes: {
        foodAnimation: { v: true },
        showBorder: { v: true },
        showInactiveTabBorder: { v: true },
        autoTheme: { v: false },
    },

    themeComputed: {
        activeColor: new Float32Array(3),
        inactiveColor: new Float32Array(3),
        foodColor: new Float32Array(3),
        ejectColor: new Float32Array(3),
    },

    flags: new Uint8Array(2),
    pids: new Int16Array(2),
    cellCts: new Uint16Array(2),
    vports: new Float32Array(4),
    scores: new Float32Array(2),
    map: new Float32Array(2),

    timings: {},
};

// const Renderer = require("./build/Debug/gfx-addon.node");

// Renderer.postInit(client);

// setInterval(() => Renderer.render(client, 0, 100), 100);

const Cytos = require("./build/Debug/cytos-addon.node");

// Cytos.onBuffer((buf) => {
//     Renderer.parse(client, buf.buffer);
// });

Cytos.onInfo(({ event, id, pid0, pid1 }) => {
    if (event === "join") {
        if (id === "bot") {
            client.playerData.set(pid0, { isBot: true, pid: pid0 });
        } else if (id === "player") {
            pid0 &&
                client.playerData.set(pid0, {
                    pid: pid0,
                    skinTex: tex(),
                    nameTex: tex(),
                });
            pid1 &&
                client.playerData.set(pid1, {
                    pid: pid1,
                    skinTex: tex(),
                    nameTex: tex(),
                });
        }
    }
});

Cytos.setGameMode("omega");

setInterval(() => {
    const timings = Cytos.getTimings();
    console.log(timings);
}, 1000);

setTimeout(() => {
    Cytos.setInput({
        spectate: false,
        activeTab: 0,
        data: [
            {
                line: false,
                spawn: true,
                macro: false,
                splits: 0,
                ejects: 0,
                mouseX: 0,
                mouseY: 0,
            },
            {
                line: false,
                spawn: false,
                macro: false,
                splits: 0,
                ejects: 0,
                mouseX: 0,
                mouseY: 0,
            },
        ],
    });
}, 1000);

// Test save & restore
// setTimeout(() => {
//     const output = Cytos.save();
//     console.log(output);

//     setTimeout(() => {
//         const restored = Cytos.restore(output.mode, output.buffer);
//         console.log(restored);
//     }, 1000);
// }, 2000);
