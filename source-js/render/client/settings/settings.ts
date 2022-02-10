import { HUDStore } from '../../stores/hud';
import { Setting } from './setting';

export class S<T> {
    readonly k: string;
    v: T;
    constructor(key: string, value: T) {
        this.k = key;
        this.v = value;
    }
}

const s = {
    renderSkin: new S('renderSkin', true),
    renderName: new S('renderName', true),
    renderMass: new S('renderMass', 1),
    renderFood: new S('renderFood', true),
    drawDelay: new S('drawDelay', 120),
    resolution: new S('resolution', 1),
    nameThresh: new S('nameThresh', 25),
    massThresh: new S('massThresh', 25),
    showHUD: new S('showHUD', true),
    showStats: new S('showStats', true),
    showChat: new S('showChat', true),
    showLB: new S('showLB', true),
    showMM: new S('showMM', true),
    replayFPS: new S('replayFPS', 1),
    replayBitrate: new S('replayBitrate', 1),
    cameraSpeed: new S('cameraSpeed', 3),
    cameraMode: new S('cameraMode', 0),
};

export const SettingCategory: { [key: string]: Setting<number | boolean>[] } = {
    Render: [
        new Setting(s.renderSkin, 0, {
            text: 'Render Skins',
        }),
        new Setting(s.renderName, 0, {
            text: 'Render Names',
        }),
        new Setting(
            s.renderMass,
            1,
            {
                min: 0,
                max: 2,
                step: 1,
                text: 'Render Mass',
            },
            {
                0: 'Render Mass: None',
                1: 'Render Mass: Short',
                2: 'Render Mass: Long',
            },
        ),
        new Setting(s.renderFood, 0, {
            text: 'Render Food',
        }),
        new Setting(s.drawDelay, 3, {
            text: 'Animation Delay',
            min: 10,
            max: 300,
            step: 10,
        }),
        new Setting(
            s.resolution,
            1,
            {
                min: 0,
                max: 3,
                step: 1,
                text: 'Resolution',
            },
            {
                0: '3840x2160 Ultra HD',
                1: '1920x1080 Full HD',
                2: '1280x720 HD',
                3: '256x144 PH',
            },
        ),
    ],
    Camera: [
        new Setting(s.cameraSpeed, 3, {
            text: 'Camera Speed',
            min: 1,
            max: 10,
            step: 1,
        }),
        new Setting(
            s.cameraMode,
            1,
            {
                text: 'Camera Mode',
                min: 0,
                max: 1,
                step: 1,
            },
            {
                0: 'Viewports: Combined',
                1: 'Viewports: Separate',
            },
        ),
    ],
    HUD: [
        new Setting(s.showHUD, 0, {
            text: 'Show HUD',
            state: HUDStore.visible.all,
        }),
        new Setting(s.showChat, 0, {
            text: 'Show Chat',
            dep: ['showHUD'],
            state: HUDStore.visible.chat,
        }),
        new Setting(s.showStats, 0, {
            text: 'Show Stats',
            dep: ['showHUD'],
            state: HUDStore.visible.stats,
        }),
        new Setting(s.showLB, 0, {
            text: 'Show Leaderboard',
            dep: ['showHUD'],
            state: HUDStore.visible.lb,
        }),
        new Setting(s.showMM, 0, {
            text: 'Show Minimap',
            dep: ['showHUD'],
            state: HUDStore.visible.mm,
        }),
    ],
    Replay: [
        new Setting(
            s.replayFPS,
            1,
            {
                min: 0,
                max: 2,
                step: 1,
                text: 'Replay FPS',
            },
            {
                0: 'Replay FPS: 15',
                1: 'Replay FPS: 30',
                2: 'Replay FPS: 60',
            },
        ),
        new Setting(
            s.replayBitrate,
            1,
            {
                min: 0,
                max: 2,
                step: 1,
                text: 'Replay Bitrate',
            },
            {
                0: 'Replay Bitrate: Low',
                1: 'Replay Bitrate: Medium',
                2: 'Replay Bitrate: High',
            },
        ),
    ],
};

export const SettingList = Object.values(SettingCategory).reduce(
    (prev, curr) => prev.concat(curr),
    [],
);

export const Settings: Partial<typeof s> = Object.assign({}, s);

export const getResolution = () =>
    [
        [3840, 2160],
        [1920, 1080],
        [1280, 720],
        [256, 144],
    ][Settings.resolution.v];
