import { Panels } from './panels/panels';
import { Stats } from './hud/stats';
import { NerdStats } from './hud/nerdstats';
import { ChatBox } from './hud/chat/chatbox';
import { ChatInput } from './hud/chat/chatinput';
import { ToastContainer } from 'react-toastify';
import ReactTooltip from 'react-tooltip';
import 'react-toastify/dist/ReactToastify.css';

import Style from '../css/overlay.module.css';
import { useEffect, useState } from 'react';
import Client from '../../client';
import { useState as useStore } from '@hookstate/core';
import { HUDStore } from '../../stores/hud';
import { GameContextMenu } from './hud/game-context';

const Overlay = () => {
    const menu = useStore(HUDStore.menu);
    const [show, setShow] = useState(true);

    useEffect(() => {
        if (menu.value) {
            setShow(true);
            return () => {};
        } else {
            if (Client.instance) Client.instance.canvas.focus();
            const timeout = setTimeout(() => setShow(false), 1000);
            return () => clearTimeout(timeout);
        }
    }, [menu.value]);

    return (
        <div
            panel-transition={menu.value ? 'fade-in' : 'fade-out'}
            className={Style.overlay}
            style={{ pointerEvents: menu.value ? 'all' : 'none' }}
            hidden={!show}
        >
            <Panels showing={menu.value} />
        </div>
    );
};

export const CytosUI = () => {
    return (
        <>
            <Stats />
            <ChatBox />
            <ChatInput />
            <NerdStats />
            <ToastContainer position="top-center" autoClose={10000} pauseOnHover />

            <Overlay />
            <ReactTooltip backgroundColor="#0e1824" />
            <GameContextMenu />
        </>
    );
};
