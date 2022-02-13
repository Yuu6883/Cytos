import { useEffect, useState } from 'react';
import { Tab, Tabs, TabList, TabPanel } from 'react-tabs';

import { Panel } from '../panel';
import { ServersList } from '../content/serverlist/serversList';

import NavStyle from '../../../css/nav.module.css';
import { useState as useStore } from '@hookstate/core';
import { CurrServer, OnGameDisconnect } from '../../../../stores/servers';
import { ServerPanel } from './views/server';
import ReactTooltip from 'react-tooltip';
import { SYS } from '../../../../client/util/sys_message';
import Client from '../../../../client';

export const RightPanel = (props: { transition?: string }) => {
    const { connected, name } = useStore(CurrServer).value;

    const [cooldown, setCooldown] = useState(false);

    useEffect(() => {
        if (connected) {
            setCooldown(true);
            const timeout = setTimeout(() => setCooldown(false), 500);
            return () => clearTimeout(timeout);
        } else return () => {};
    }, [connected]);

    return (
        <Panel
            style={{
                minWidth: '160px',
            }}
            transition={props.transition}
        >
            <Tabs
                onSelect={() => {}}
                selectedIndex={Number(connected)}
                selectedTabClassName={NavStyle.activeTab}
            >
                <TabList className={NavStyle.nav}>
                    <Tab hidden disabled></Tab>
                    <Tab hidden={!connected} disabled>
                        {name.toUpperCase()}{' '}
                        <i
                            onClick={() => {
                                if (!cooldown) {
                                    Client.instance.connect(null);
                                    SYS.gameDisconnected(name.toLocaleUpperCase());
                                    OnGameDisconnect();
                                    ReactTooltip.hide();
                                }
                            }}
                            data-tip="Disconnect"
                            className="fas fa-sign-out-alt"
                            style={{
                                float: 'right',
                                fontSize: '24px',
                                cursor: cooldown ? 'not-allowed' : 'pointer',
                            }}
                        ></i>
                    </Tab>
                </TabList>
                <TabPanel>
                    <ServersList />
                </TabPanel>
                <TabPanel>
                    <ServerPanel />
                </TabPanel>
            </Tabs>
        </Panel>
    );
};
