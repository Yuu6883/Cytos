import { useEffect, useState } from 'react';
import { useState as useStore } from '@hookstate/core';
import { scoreToString, IStats, HUDStore } from '../../../stores/hud';

import Client from '../../../client';

import Style from '../../css/hud.module.css';
import { getUIColorStyle } from '../../../client/util/colors';

const StatsItem = (props: { k: string; value: string | number; color?: string }) => (
    <p className={Style.statsItem}>
        <span style={getUIColorStyle()}>{props.k}</span>
        <span style={{ color: props.color || 'white' }}>{props.value}</span>
    </p>
);

const LINE_COLORS = ['', '#f5515b', '#4fe5ff', '#ffff4f'];
const LINE_VALUES = ['OFF', 'DIAGONAL', 'HORIZONTAL', 'VERTICAL'];

export const Stats = () => {
    const v = useStore(HUDStore.visible).value;
    const [{ line, cells, fps, score }, setState] = useState<IStats>({
        fps: 0,
        line: 0,
        cells: 0,
        score: 0,
    });

    useEffect(() => {
        const interval = setInterval(
            () => Client.instance && setState(Object.assign({}, Client.instance.stats)),
            100,
        );

        return () => clearInterval(interval);
    }, []);

    return v.all && v.stats ? (
        <div className={Style.stats}>
            <div>
                <StatsItem k="FPS" value={~~fps} />
                <StatsItem k="LINE" value={LINE_VALUES[line]} color={LINE_COLORS[line]} />
                <StatsItem k="CELL" value={cells} />
                <StatsItem k="MASS" value={scoreToString(score)} />
            </div>
        </div>
    ) : (
        <></>
    );
};
