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

import AppIcon from '../../../cytos.ico';

const NativeBar = () => (
    <nav className="uk-navbar-container" id="app-nav" uk-navbar>
        <div className="uk-navbar-left uk-margin-small-left">
            <img id="app-icon" src={AppIcon} />
            &nbsp;&nbsp;
            <span id="app-title"> Cytos </span>
        </div>
        <div className="uk-navbar-right">
            <ul className="uk-iconnav">
                <li className="title-li">
                    <span
                        className="title-button"
                        uk-icon="minus"
                        id="minimize-button"
                    ></span>
                </li>
                <li className="title-li">
                    <span
                        className="title-button"
                        uk-icon="expand"
                        id="toggle-button"
                    ></span>
                </li>
                <li className="title-li">
                    <span
                        className="title-button"
                        uk-icon="close"
                        id="close-button"
                    ></span>
                </li>
            </ul>
        </div>
    </nav>
);

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
            <NativeBar />
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
