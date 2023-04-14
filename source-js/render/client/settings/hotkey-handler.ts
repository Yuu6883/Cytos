// credit to nebula for this file totally didnt take it from him lmao

import Client from '..';

import { Hotkeys, HotkeysList } from './keybinds';
import { Mouse, MiceList } from './mouse';
import { SettingList } from './settings';
import { HUDStore } from '../../stores/hud';
import { ThemeList, updateCursor } from './themes';

const EventType = {
    NONE: 0,
    FEED_MACRO: 1,
    SPLIT: 2,
    DOUBLE_SPLIT: 3,
    TRIPLE_SPLIT: 4,
    X16_SPLIT: 5,
    X32_SPLIT: 6,
    X64_SPLIT: 7,
    X128_SPLIT: 8,
    X256_SPLIT: 9,
    MAX_SPLIT: 10,
    SWITCH_TAB: 11,
    RESPAWN: 12,
    LINE_SPLIT: 13,
};

export class HotkeyHandler {
    public static keyMap: Map<string, string>;
    private static activeKeys: Set<string>;

    public static init() {
        this.setKeyMap();
        this.activeKeys = new Set();

        window.addEventListener('keydown', this.onKeyDown.bind(this));
        window.addEventListener('keyup', this.onKeyUp.bind(this));

        window.addEventListener('mousedown', this.onMouse.bind(this, false));
        window.addEventListener('mouseup', this.onMouse.bind(this, true));

        window.addEventListener('contextmenu', this.onContextMenu.bind(this));

        MiceList.forEach(hotkey => hotkey.init());
        HotkeysList.forEach(hotkey => hotkey.init());
        SettingList.forEach(setting => setting.init());
        ThemeList.forEach(theme => theme.init());

        updateCursor();
    }

    public static onContextMenu(e: MouseEvent) {
        e.preventDefault();
        e.stopPropagation();
    }

    public static onMouse(up: boolean, e: MouseEvent) {
        if (!e.isTrusted) return;
        if (!Client.instance) return;
        // e.preventDefault();
        const type = this.getMouseType(e);
        const mouse = Mouse[type];
        if (!mouse) return;

        const c = Client.instance;
        const p = c.input;
        const ctxOpen = HUDStore.ctxMenu.open;

        const elem = e.target as HTMLElement;

        // Request spectate (clicked on canvas AND ctx menu is not open)
        if (!up && !ctxOpen.value && type === 'left' && elem.tagName === 'CANVAS')
            c.spectateAtCursor();

        if (up) {
            // Close game context menu
            if (ctxOpen.value) ctxOpen.set(false);
            if (mouse.v >= 0 && type === 'right') {
                if (elem.tagName === 'CANVAS') {
                    const data = c.getUserAtCursor();
                    if (!data.empty) {
                        setTimeout(() => {
                            HUDStore.ctxMenu.set({
                                open: true,
                                x: e.clientX,
                                y: e.clientY,
                                data: data,
                            });
                        });
                    }
                }
            }
        } else {
            window.getSelection?.().removeAllRanges();
        }

        if (mouse.v < 0 || HUDStore.menu.value) return;

        if (up && mouse.v === EventType.FEED_MACRO) {
            p.macro = false;
        } else if (!up) {
            switch (mouse.v) {
                case EventType.SPLIT:
                    p.splits += 1;
                    break;
                case EventType.DOUBLE_SPLIT:
                    p.splits += 2;
                    break;
                case EventType.TRIPLE_SPLIT:
                    p.splits += 3;
                    break;
                case EventType.X16_SPLIT:
                    p.splits += 4;
                    break;
                case EventType.X32_SPLIT:
                    p.splits += 5;
                    break;
                case EventType.X64_SPLIT:
                    p.splits += 6;
                    break;
                case EventType.X128_SPLIT:
                    p.splits += 7;
                    break;
                case EventType.X256_SPLIT:
                    p.splits += 8;
                    break;
                case EventType.MAX_SPLIT:
                    p.splits = 12;
                    break;
                case EventType.FEED_MACRO:
                    p.macro = true;
                    break;
                case EventType.SWITCH_TAB:
                    if (!p.isDual) break;
                    c.switchTab();
                    break;
                case EventType.RESPAWN:
                    p.spawn = true;
                    break;
                case EventType.LINE_SPLIT:
                    p.line = true;
                    break;
            }
        }
    }

    public static getMouseType(e: MouseEvent): string {
        let type = '';
        switch (e.button) {
            case 0:
                type = 'left';
                break;
            case 1:
                type = 'middle';
                break;
            case 2:
                type = 'right';
                break;
            default:
                type = `mouse${e.button + 1}`;
                break;
        }
        return type;
    }

    public static onKeyDown(e: KeyboardEvent): void {
        if (!e.isTrusted) return;

        const key = this.getKeyString(e);

        const t = e.target as HTMLElement;
        if (t.hasAttribute && t.hasAttribute('chat-input')) return;
        if (['NumpadEnter', 'Enter'].includes(e.code))
            return HUDStore.chatInput.set(!HUDStore.chatInput.value);

        if (key === 'TAB') e.preventDefault();

        if (!key || this.activeKeys.has(key)) return;
        this.activeKeys.add(key);

        if (key === 'ESC') return HUDStore.menu.set(!HUDStore.menu.value);

        if (HUDStore.menu.value) return;

        const c = Client?.instance;
        const p = c.input;

        switch (key) {
            case Hotkeys.split1.v:
                p.splits += 1;
                break;
            case Hotkeys.split2.v:
                p.splits += 2;
                break;
            case Hotkeys.split3.v:
                p.splits += 3;
                break;
            case Hotkeys.split4.v:
                p.splits += 4;
                break;
            case Hotkeys.split5.v:
                p.splits += 5;
                break;
            case Hotkeys.split6.v:
                p.splits += 6;
                break;
            case Hotkeys.split7.v:
                p.splits += 7;
                break;
            case Hotkeys.split8.v:
                p.splits += 8;
                break;
            case Hotkeys.splitA.v:
                p.splits = 9;
                break;
            case Hotkeys.macro.v:
                p.macro = true;
                break;
            case Hotkeys.switchTab.v:
                if (!p.isDual) break;
                c.switchTab();
                break;
            case Hotkeys.respawn.v:
                p.spawn = true;
                break;
            case Hotkeys.lineSplit.v:
                p.line = true;
                break;
            case Hotkeys._8xline.v:
                p.new_line(3);
                break;
            case Hotkeys._16xline.v:
                p.new_line(4);
                break;
            case Hotkeys.toggleSkin.v:
                const skinOP = SettingList.find(o => o.ref.k === 'renderSkin');
                skinOP.v = !skinOP.v;
                break;
            case Hotkeys.toggleName.v:
                const nameOP = SettingList.find(o => o.ref.k === 'renderName');
                nameOP.v = !nameOP.v;
                break;
            case Hotkeys.toggleMass.v:
                const massOP = SettingList.find(o => o.ref.k === 'renderMass');
                massOP.v = ((massOP.v as number) + 1) % 3;
                break;
            case Hotkeys.toggleHUD.v: {
                const hudOP = SettingList.find(o => o.ref.k === 'showHUD');
                hudOP.v = !hudOP.v;
                break;
            }
            case Hotkeys.toggleBorder.v: {
                const borderOP = ThemeList.find(o => o.ref.k === 'showBorder');
                borderOP.v = !borderOP.v;
                break;
            }
            case Hotkeys.zoomIn.v:
                c.mouse.scroll -= 100;
                break;
            case Hotkeys.zoomOut.v:
                c.mouse.scroll += 100;
                break;
            case Hotkeys.save.v:
                c.save();
                break;
            case Hotkeys.restore.v:
                c.restore();
                break;
            case 'F1':
                HUDStore.nerdVisible.set(!HUDStore.nerdVisible.value);
                break;
            case 'F4':
                c.playVid();
                break;

            // minion
            case Hotkeys.split1_minion.v:
                p.minion_splits += 1;
                break;
            case Hotkeys.split2_minion.v:
                p.minion_splits += 2;
                break;
            case Hotkeys.split3_minion.v:
                p.minion_splits += 3;
                break;
            case Hotkeys.split4_minion.v:
                p.minion_splits += 4;
                break;
            case Hotkeys.split5_minion.v:
                p.minion_splits += 5;
                break;
            case Hotkeys.split6_minion.v:
                p.minion_splits += 6;
                break;
            case Hotkeys.split7_minion.v:
                p.minion_splits += 7;
                break;
            case Hotkeys.split8_minion.v:
                p.minion_splits += 8;
                break;
            case Hotkeys.splitA_minion.v:
                p.minion_splits = 9;
                break;
            case Hotkeys.macro_minion.v:
                p.minion_macro = true;
                break;
            case Hotkeys.respawn_minion.v:
                p.spawn = true;
                break;
            case Hotkeys.lineSplit_minion.v:
                p.minion_line = true;
                break;
            case Hotkeys._8xline_minion.v:
                p.new_line_minion(3);
                break;
            case Hotkeys._16xline_minion.v:
                p.new_line_minion(4);
                break;
        }
    }

    public static onKeyUp(e: KeyboardEvent): void {
        if (!e.isTrusted) return;

        const key = this.getKeyString(e);
        this.activeKeys.delete(key);

        const p = Client.instance && Client.instance.input;

        if (!p) return;
        if (key) {
            if (key === Hotkeys.macro.v) p.macro = false;
            else if (key === Hotkeys.macro_minion.v) p.minion_macro = false;
        }
    }

    public static getKeyString(e: KeyboardEvent) {
        if (!e.key) return '';
        const key = this.keyMap.get(e.code);
        if (!key) {
            if (/^F[1-4]$/.test(e.code)) {
                e.preventDefault();
                return e.key;
            } else return null;
        }

        let prefix = '';
        if (key.length === 1) {
            if (e.ctrlKey) {
                prefix += 'CTRL+';
            }
            if (e.altKey) {
                prefix += 'ALT+';
            }
            if (e.metaKey) {
                prefix += 'META+';
            }
        }
        return prefix + key;
    }

    public static setKeyMap() {
        this.keyMap = new Map([
            ['CapsLock', 'CAPS'],
            ['Escape', 'ESC'],
        ]);

        'ABCDEFGHIJKLMNOPQRSTUVWXYZ'.split('').forEach(key => {
            this.keyMap.set('Key' + key, key); // simple keys
        });

        [
            ...['Backslash', 'Quote', 'Semicolon', 'Slash', 'Comma', 'Period'],
            ...['ContextMenu', 'Tab', 'Backquote', 'Minus', 'Equal', 'Delete'],
            ...['Backspace', 'NumLock', 'Space'],
        ].forEach(key => this.keyMap.set(key, key.toUpperCase())); // the rest

        ['Bracket Right', 'Bracket Left', 'Shift Right', 'Shift Left'].forEach(key => {
            const wtfAmIDoing = key.split(' ');
            this.keyMap.set(wtfAmIDoing.join(''), wtfAmIDoing[1][0] + wtfAmIDoing[0]); // Left/Right keys
        });

        '0123456789'.split('').forEach(digit => {
            this.keyMap.set('Digit' + digit, digit); // normal digits
            this.keyMap.set('Numpad' + digit, 'KP' + digit); // Numpad(KeyPad) digits
        });

        ['Decimal', 'Add', 'Subtract', 'Multiply', 'Divide'].forEach(key => {
            this.keyMap.set('Numpad' + key, 'KP' + key); // the rest of numpad keys
        });

        ['Up', 'Left', 'Right', 'Down'].forEach(key => {
            this.keyMap.set('Arrow' + key, key);
        });
    }
}
