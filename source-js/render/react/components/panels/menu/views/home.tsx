import ReactToolTip from 'react-tooltip';
import Client from '../../../../../client';

import Style from '../../../../css/home.module.css';
import { useState } from '@hookstate/core';
import { SaveInputs, InputStore, defaultSkin } from '../../../../../stores/inputs';
import { CurrServer } from '../../../../../stores/servers';
import { HUDStore } from '../../../../../stores/hud';
import { useEffect } from 'react';

interface Props {
    setIndex: (index: number) => void;
}

export const Home = ({ setIndex }: Props) => {
    const connected = useState(CurrServer.connected);
    const inputs = useState(InputStore);

    const name = inputs.name;
    const skin1 = inputs.skin1;
    const skin2 = inputs.skin2;

    const c = Client.instance;

    useEffect(() => {
        if (!c) return;
        c.myPlayerData[0]?.update(name.value, skin1.value);
        c.myPlayerData[1]?.update(name.value, skin2.value);
    }, [name.value, skin1.value, skin2.value]);

    return (
        <>
            <div onMouseEnter={() => ReactToolTip.rebuild()} className={Style.preview}>
                <img
                    data-tip="Change Skin"
                    className="rainbow"
                    src={skin1.value || defaultSkin}
                    alt="Skin"
                    onClick={() => setIndex(4)}
                    onError={() => skin1.set(defaultSkin)}
                />
                {skin2.value && skin2.value !== skin1.value && (
                    <img
                        data-tip="Change Skin"
                        className={`rainbow ${Style.secondarySkin}`}
                        src={skin2.value}
                        alt="Skin"
                        onClick={() => setIndex(4)}
                        onError={() => skin2.set(null)}
                    />
                )}
            </div>

            <div className={Style.content}>
                <input
                    key="enabled-input"
                    type="text"
                    placeholder="Name"
                    defaultValue={name.value}
                    onChange={e => name.set(e.target.value)}
                    onBlur={SaveInputs}
                    spellCheck={false}
                    maxLength={16}
                />
                <button
                    onClick={() => {
                        HUDStore.menu.set(false);
                        c?.continueOrRespawn();
                    }}
                    className={Style.play}
                    disabled={!connected.value}
                >
                    Play
                </button>

                <button
                    onClick={() => {
                        HUDStore.menu.set(false);
                        c?.spectateAtCursor(true);
                    }}
                    className={Style.spectate}
                    disabled={!connected.value}
                    data-tip="Spectate"
                >
                    <i className="far fa-eye"></i>
                </button>
            </div>
        </>
    );
};
