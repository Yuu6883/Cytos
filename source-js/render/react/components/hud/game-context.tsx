import { useState } from '@hookstate/core';
import { HUDStore } from '../../../stores/hud';
import { defaultSkin, GetInputs } from '../../../stores/inputs';
import BotGIF from '../../../img/bot.gif';
import CYTGIF from '../../../img/cyt.gif';
import EXPGIF from '../../../img/exp.gif';
import Style from '../../css/hud.module.css';
import Client from '../../../client';
import React, { useEffect } from 'react';
import { getUIColorStyle } from '../../../client/util/colors';
import { GuestName } from '../../../client/util';

const ContextMenu = () => {
    const c = Client.instance;
    const ctx = useState(HUDStore.ctxMenu);
    const spectating = useState(HUDStore.spectating).value;

    let img: string;
    let name: string;
    let imgClass = Style.ctxImg;

    const data = ctx.data.value;

    let colorStyle: React.CSSProperties = {};

    if (data.unknown) {
        img = defaultSkin;
        name = '???';
    } else if (data.bot) {
        img = BotGIF;
        name = 'Bot';
    } else if (data.cyt) {
        img = CYTGIF;
        name = 'CYT (Currency)';
    } else if (data.exp) {
        img = EXPGIF;
        name = 'EXP';
    } else if (data.id <= 0) {
        name = GuestName(-data.id);
    } else {
        const [n, skin] = GetInputs();
        img = skin;
        name = n;
        imgClass += ' rainbow';
        colorStyle = getUIColorStyle();
    }

    // have pid AND spectating someone OR not alive
    const canSpectate = !!data.pid && spectating;

    const spectate = () => c.spectate(data.pid);

    const showSkin = () => Client.instance.updateSkinState(data?.pid, 1);
    const hideSkin = () => Client.instance.updateSkinState(data?.pid, 0);

    const cb: (func: Function) => React.MouseEventHandler = func => e => {
        e.stopPropagation();
        func();
        ctx.open.set(false);
    };

    const xTransform = ctx.x.value > window.innerWidth / 2 ? -100 : 0;
    const yTransform = ctx.y.value > window.innerHeight / 2 ? -100 : 0;

    return (
        <div
            className={Style.ctxMenu}
            style={{
                left: `${ctx.x.value}px`,
                top: `${ctx.y.value}px`,
                transform: `translate(${xTransform}%, ${yTransform}%)`,
            }}
        >
            <div
                style={{ padding: '10px' }}
                className={data.offline ? Style.ctxOffline : ''}
            >
                <img
                    src={img}
                    onError={e => (e.currentTarget.src = defaultSkin)}
                    className={imgClass}
                    style={{ display: 'none' }}
                    onLoad={e => (e.currentTarget.style.display = '')}
                />
                <span className={Style.ctxTitle} style={colorStyle}>
                    {name}
                </span>
            </div>
            {canSpectate && (
                <div onMouseUp={cb(spectate)} className={Style.ctxField}>
                    Spectate
                </div>
            )}
            {data?.skinState === 0 && (
                <div onMouseUp={cb(showSkin)} className={Style.ctxField}>
                    Show Skin
                </div>
            )}
            {data?.skinState === 1 && (
                <div onMouseUp={cb(hideSkin)} className={Style.ctxField}>
                    Hide Skin
                </div>
            )}
        </div>
    );
};

export const GameContextMenu = () => {
    const open = useState(HUDStore.ctxMenu.open);
    const showMenu = useState(HUDStore.menu).value;

    useEffect(() => open.set(false), [showMenu]);

    return open.value ? <ContextMenu /> : <></>;
};
