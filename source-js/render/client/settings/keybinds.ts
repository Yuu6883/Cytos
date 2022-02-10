// credit to nebula for this file totally didnt take it from him lmao

import { Setting } from './setting';
import { S } from './settings';

const s = {
    macro: new S('macro', 'W'),
    split1: new S('split1', 'SPACE'),
    split2: new S('split2', 'G'),
    split3: new S('split3', 'Z'), // x8
    split4: new S('split4', 'T'), // x16
    split5: new S('split5', ''), // x32
    split6: new S('split6', ''), // x64
    split7: new S('split7', ''), // x128
    split8: new S('split8', ''), // x256
    splitA: new S('splitA', ''), // split all
    switchTab: new S('switchTab', 'TAB'),
    respawn: new S('respawn', 'N'),
    lineSplit: new S('lineSplit', 'F'),
    toggleSkin: new S('toggleSkin', ''),
    toggleName: new S('toggleName', ''),
    toggleMass: new S('toggleMass', ''),
    toggleMinimap: new S('toggleMinimap', 'M'),
    toggleHUD: new S('toggleHUD', 'U'),
    toggleBorder: new S('toggleBorder', ''),
    zoomIn: new S('zoomIn', ''),
    zoomOut: new S('zoomOut', ''),
    save: new S('save', 'CTRL+S'),
    restore: new S('restore', 'CTRL+R'),
    // minion
    macro_minion: new S('macro_minion', ''),
    split1_minion: new S('split1_minion', ''),
    split2_minion: new S('split2_minion', ''),
    split3_minion: new S('split3_minion', ''), // x8
    split4_minion: new S('split4_minion', ''), // x16
    split5_minion: new S('split5_minion', ''), // x32
    split6_minion: new S('split6_minion', ''), // x64
    split7_minion: new S('split7_minion', ''), // x128
    split8_minion: new S('split8_minion', ''), // x256
    splitA_minion: new S('splitA_minion', ''), // split all
    respawn_minion: new S('respawn_minion', ''),
    lineSplit_minion: new S('lineSplit_minion', ''),
    minionMode: new S('minionMode', 0),
    persistentMacro: new S('persistentMacro', true),
    autoUnlockLine: new S('autoUnlockLine', false),
    autoUnlockLineTimeout: new S('autoUnlockLineTimeout', 1000),
};

export const HotkeyCategory: { [key: string]: Setting<number | boolean | string>[] } = {
    Tab: [
        new Setting(s.switchTab, 5, {
            text: 'Switch Tab',
        }),
    ],
    Feed: [
        new Setting(s.macro, 5, {
            text: 'Feed Macro',
            minion: s.macro_minion,
        }),
        new Setting(s.persistentMacro, 0, {
            text: 'Persistent Macro',
        }),
    ],
    Split: [
        new Setting(s.split1, 5, {
            text: 'Split',
            minion: s.split1_minion,
        }),
        new Setting(s.split2, 5, {
            text: 'Double Split',
            minion: s.split2_minion,
        }),
        new Setting(s.split3, 5, {
            text: 'Triple Split',
            minion: s.split3_minion,
        }),
        new Setting(s.split4, 5, {
            text: 'X16 Split',
            minion: s.split4_minion,
        }),
        new Setting(s.split5, 5, {
            text: 'X32 Split',
            minion: s.split5_minion,
        }),
        new Setting(s.split6, 5, {
            text: 'X64 Split',
            minion: s.split6_minion,
        }),
        new Setting(s.split7, 5, {
            text: 'X128 Split',
            minion: s.split7_minion,
        }),
        new Setting(s.split8, 5, {
            text: 'X256 Split',
            minion: s.split8_minion,
        }),
        new Setting(s.splitA, 5, {
            text: 'Max Split',
            minion: s.splitA_minion,
        }),
    ],
    Line: [
        new Setting(s.lineSplit, 5, {
            text: 'Line Lock',
            minion: s.lineSplit_minion,
        }),
        new Setting(s.autoUnlockLine, 0, {
            text: 'Auto Line Unlock',
        }),
        new Setting(s.autoUnlockLineTimeout, 3, {
            text: 'Auto Line Unlock Timeout',
            min: 500,
            max: 2000,
            step: 250,
            dep: ['autoUnlockLine'],
            multi: 0.001,
            unit: 's',
            precision: 2,
        }),
    ],
    Toggles: [
        new Setting(s.toggleSkin, 5, {
            text: 'Toggle Skin',
        }),
        new Setting(s.toggleName, 5, {
            text: 'Toggle Name',
        }),
        new Setting(s.toggleMass, 5, {
            text: 'Toggle Mass',
        }),
        new Setting(s.toggleMinimap, 5, {
            text: 'Toggle Minimap',
        }),
        new Setting(s.toggleHUD, 5, {
            text: 'Toggle HUD',
        }),
        new Setting(s.toggleBorder, 5, {
            text: 'Toggle Cell Border',
        }),
    ],
    Misc: [
        new Setting(s.respawn, 5, {
            text: 'Respawn',
            minion: s.respawn_minion,
        }),
        new Setting(s.save, 5, {
            text: 'Save Game',
        }),
        new Setting(s.restore, 5, {
            text: 'Restore Game',
        }),
        new Setting(s.zoomIn, 5, {
            text: 'Zoom In',
        }),
        new Setting(s.zoomOut, 5, {
            text: 'Zoom Out',
        }),
        new Setting(
            s.minionMode,
            1,
            {
                min: 0,
                max: 1,
                step: 1,
            },
            {
                0: 'Dual Controls',
                1: 'Minion Controls',
            },
        ),
    ],
};

export const HotkeysList = Object.values(HotkeyCategory).reduce(
    (prev, curr) => prev.concat(curr),
    [],
);

export const Hotkeys: Partial<typeof s> = Object.assign({}, s);
