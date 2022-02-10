import Style from '../../css/hud.module.css';
import { HUDStore, prettyBytes, prettyTime } from '../../../stores/hud';
import { useState } from '@hookstate/core';
import { CytosTimings } from '../../../types';

const NerdStatsItem = (props: {
    k: string;
    value: string | number;
    style?: React.CSSProperties;
}) => (
    <p className={Style.nerdItem} style={props.style}>
        <span>{props.k}:</span>
        <span>{props.value || 'N/A'}</span>
    </p>
);

const ms = (n: number) => `${n.toFixed(3)} ms`;

// Physics phase names
const P = [
    'Player-Self: Collision & Eat',
    'Player-Other Player: Eat',
    'Dead Cells: Collision',
    'Player-Ejected & Virus: Eat & Pop',
    'Player-Pellet: Eat',
    'Tree Update & Remove Player Cells',
    'Ejected-Ejected & Virus: Collision & Eat',
    'Post-Resolve',
];

const Timings = ({ timings: t }: { timings: CytosTimings }) => {
    const color = t.usage < 0.5 ? '#1fde68' : t.usage < 0.8 ? '#f5db4c' : '#f22951';

    return (
        <>
            <NerdStatsItem
                k="Engine Load"
                value={(t.usage * 100).toFixed(2) + '%'}
                style={{ color, marginTop: '25px' }}
            />
            <NerdStatsItem k="Threads" value={t.threads} />
            <NerdStatsItem k="Spawn Cells" value={ms(t.spawn_cells)} />
            <NerdStatsItem k="Handle IO" value={ms(t.handle_io)} />
            <NerdStatsItem k="Spawn Handles" value={ms(t.spawn_handles)} />
            <NerdStatsItem k="Update Cells" value={ms(t.update_cells)} />
            <NerdStatsItem
                k="Physics Total"
                value={ms(t.resolve_physics)}
                style={{ marginBottom: '10px' }}
            />
            {t.physics.map((k, i) => (
                <NerdStatsItem key={i} k={P[i]} value={ms(k)} />
            ))}
        </>
    );
};

export const NerdStats = () => {
    const nerdVisible = useState(HUDStore.nerdVisible).value;
    const stats = useState(HUDStore.nerdStats).value;

    return (
        <div className={Style.nerd} hidden={!nerdVisible}>
            <NerdStatsItem k="Version" value={stats.version} />
            <NerdStatsItem k="Uptime" value={prettyTime(stats.uptime)} />
            <NerdStatsItem k="Compile" value={stats.compile} />
            <NerdStatsItem k="Map" value={stats.map.join(' x ')} />
            <NerdStatsItem k="Render Cells" value={stats.rendercells} />
            <NerdStatsItem
                k="Mem Bandwidth"
                value={prettyBytes(stats.bandwidth) + '/s'}
            />
            <NerdStatsItem
                k="GPU Bandwidth"
                value={prettyBytes(stats.gpuBandwidth) + '/s'}
            />
            {stats.timings && <Timings timings={stats.timings} />}
            <p>Press F1 to hide stats</p>
        </div>
    );
};
