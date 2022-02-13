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
        <span>
            {props.value
                ? props.value
                : isNaN(props.value as number)
                ? 'N/A'
                : props.value}
        </span>
    </p>
);

const ms = (n: number) => `${n.toFixed(3)} ms`;

// IO phase names
const IO = ['Handle Inputs', 'Tree Restructure', 'Player/Bot OnTick'];

// Physics phase names
const PHY = [
    'Player-Self: Collision & Eat',
    'Player-Other Player: Eat',
    'Dead Cells: Collision',
    'Player-Ejected & Virus: Eat & Pop',
    'Player-Pellet: Eat',
    'Tree Update & Remove Player Cells',
    'Ejected-Ejected & Virus: Collision & Eat',
    'Post-Resolve',
];

const K = (n: number, pad = 7) => `${(n / 1000).toFixed(1).padStart(pad, ' ')}K`;

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
            <details>
                <summary>Engine Timings:</summary>
                <NerdStatsItem k="Spawn Cells" value={ms(t.spawn_cells)} />
                <NerdStatsItem k="Handle IO" value={ms(t.handle_io)} />
                <NerdStatsItem k="Spawn Handles" value={ms(t.spawn_handles)} />
                <NerdStatsItem k="Update Cells" value={ms(t.update_cells)} />
                <NerdStatsItem
                    k="Physics Total"
                    value={ms(t.resolve_physics)}
                    style={{ marginBottom: '10px' }}
                />
            </details>
            <details>
                <summary>IO Timings:</summary>
                {t.io.map((k, i) => (
                    <NerdStatsItem key={i} k={IO[i]} value={ms(k)} />
                ))}
            </details>
            <details>
                <summary>Physics Timings:</summary>
                {t.physics.map((k, i) => (
                    <NerdStatsItem key={i} k={PHY[i]} value={ms(k)} />
                ))}
            </details>
            <details>
                <summary>QuadTree Stats</summary>
                <NerdStatsItem
                    k="Query1"
                    value={`${((t.queries[1] / t.queries[0]) * 100).toFixed(2)}% (${K(
                        t.queries[1],
                        4,
                    )}/${K(t.queries[0], 4)})`}
                    style={{ gridTemplateColumns: '80px auto', whiteSpace: 'pre' }}
                />
                <NerdStatsItem
                    k="Query2"
                    value={`${((t.queries[3] / t.queries[2]) * 100).toFixed(2)}% (${K(
                        t.queries[3],
                        4,
                    )}/${K(t.queries[2], 4)})`}
                    style={{ gridTemplateColumns: '80px auto', whiteSpace: 'pre' }}
                />
                <p>TreeLevels: Items | Callbacks | Efficiency</p>
                {t.tree.map((k, i) => (
                    <NerdStatsItem
                        key={i}
                        k={`TreeLevel[${i}]`}
                        value={`${k.toString().padStart(4, ' ')} | ${K(
                            t.counter[i * 2],
                        )} | ${((t.counter[i * 2 + 1] / t.counter[i * 2]) * 100)
                            .toFixed(4)
                            .padStart(7, ' ')}%`}
                        style={{ gridTemplateColumns: '80px auto', whiteSpace: 'pre' }}
                    />
                ))}
            </details>
        </>
    );
};

export const NerdStats = () => {
    const nerdVisible = useState(HUDStore.nerdVisible).value;
    const stats = useState(HUDStore.nerdStats).value;

    return (
        <div className={Style.nerd} hidden={!nerdVisible}>
            <NerdStatsItem
                k="Version"
                value={stats.version}
                style={{ gridTemplateColumns: '100px auto' }}
            />
            <NerdStatsItem
                k="Uptime"
                value={prettyTime(stats.uptime)}
                style={{ gridTemplateColumns: '100px auto' }}
            />
            <NerdStatsItem
                k="Compile"
                value={stats.compile}
                style={{ gridTemplateColumns: '100px auto' }}
            />
            <details>
                <summary>Render Stats</summary>
                <NerdStatsItem
                    k="Map"
                    value={stats.map.join(' x ')}
                    style={{ gridTemplateColumns: '120px auto' }}
                />
                <NerdStatsItem
                    k="Render Cells"
                    value={stats.rendercells}
                    style={{ gridTemplateColumns: '120px auto' }}
                />
                <NerdStatsItem
                    k="Mem Bandwidth"
                    value={prettyBytes(stats.bandwidth) + '/s'}
                    style={{ gridTemplateColumns: '120px auto' }}
                />
                <NerdStatsItem
                    k="GPU Bandwidth"
                    value={prettyBytes(stats.gpuBandwidth) + '/s'}
                    style={{ gridTemplateColumns: '120px auto' }}
                />
            </details>
            {stats.timings && <Timings timings={stats.timings} />}
            <p>Press F1 to hide stats</p>
        </div>
    );
};
