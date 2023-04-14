// credit to nebula for this file totally didnt take it from him lmao

import { Setting } from './setting';
import { S } from './settings';

export const MOUSE_ACTIONS: String[] = [
    'OFF',
    'FEED MACRO',
    'SPLIT',
    'DOUBLE SPLIT',
    'TRIPLE SPLIT',
    'X16 SPLIT',
    'X32 SPLIT',
    'X64 SPLIT',
    'X128 SPLIT',
    'X256 SPLIT',
    'MAX SPLIT',
    'SWITCH TAB',
    'RESPAWN',
    'LINE LOCK',
];

export const Mouse: { [k: string]: S<number> } = {
    left: new S('left', 0),
    middle: new S('middle', 0),
    right: new S('right', 0),
    mouse4: new S('mouse4', 0),
    mouse5: new S('mouse5', 0),
};

export const MiceList = [
    new Setting(Mouse.left, 2, {
        text: 'Left Click',
    }),
    new Setting(Mouse.middle, 2, {
        text: 'Middle Click',
    }),
    new Setting(Mouse.right, 2, {
        text: 'Right Click',
    }),
    new Setting(Mouse.mouse4, 2, {
        text: 'Mouse Button 4',
    }),
    new Setting(Mouse.mouse5, 2, {
        text: 'Mouse Button 5',
    }),
];
