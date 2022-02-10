import { setCssVar } from '../../react/utils';
import { Setting } from './setting';
import { S } from './settings';

const s = {
    autoTheme: new S('autoTheme', true),
    showBorder: new S('showBorder', true),
    showInactiveTabBorder: new S('showInactiveTabBorder', true),
    activeTabBorder: new S('activeTabBorder', '#ac2a3d'),
    inactiveTabBorder: new S('inactiveTabBorder', '#ffffff'),
    ejectColor: new S('ejectColor', '#e8385e'),
    foodColor: new S('foodColor', '#ffffff'),
    foodAnimation: new S('foodAnimation', true),
    background: new S('background', '#000000'),
    mapColor: new S('mapColor', '#111111'),
    cursor: new S('cursor', true),
};

export const ThemeList: Setting<string | boolean>[] = [
    new Setting(s.autoTheme, 0, {
        text: 'Auto Theme',
    }),
    new Setting(s.showBorder, 0, {
        text: 'Show Cell Border',
    }),
    new Setting(s.showInactiveTabBorder, 0, {
        text: 'Show Inactive Tab Border',
        dep: ['showBorder'],
    }),
    new Setting(s.activeTabBorder, 2, {
        text: 'Active Tab Border',
        dep: ['showBorder', '!autoTheme'],
    }),
    new Setting(s.inactiveTabBorder, 2, {
        text: 'Inactive Tab Border',
        dep: ['showBorder', 'showInactiveTabBorder'],
    }),
    new Setting(s.ejectColor, 2, {
        text: 'Ejected Cell Color',
        dep: ['!autoTheme'],
    }),
    new Setting(s.foodColor, 2, {
        text: 'Food Color',
    }),
    new Setting(s.foodAnimation, 0, {
        text: 'Food Animation',
    }),
    new Setting(s.cursor, 0, {
        text: 'Use Cytos Cursor',
    }),
    new Setting(s.background, 2, {
        text: 'Background Color',
    }),
    new Setting(s.mapColor, 2, {
        text: 'Map Color',
    }),
];

export const Themes: Partial<typeof s> = Object.assign({}, s);

import Cursor1 from '../../img/cytoscursor1.png';
import Cursor2 from '../../img/cytoscursor2.png';

export const updateCursor = () => {
    setCssVar(
        'use-cursor1',
        Themes.cursor.v ? `url(${Cursor1}), pointer` : 'normal-cursor',
    );
    setCssVar(
        'use-cursor2',
        Themes.cursor.v ? `url(${Cursor2}), pointer` : 'normal-cursor',
    );
};
