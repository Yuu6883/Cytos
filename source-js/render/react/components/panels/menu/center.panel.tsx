import { useState } from 'react';
import { Tab, Tabs, TabList, TabPanel } from 'react-tabs';

import { Panel } from '../panel';

import { Home } from './views/home';
import { SettingsTab } from './views/settings';
import { ThemesTab } from './views/themes';
import { Hotkeys } from './views/hotkeys';
import { Skins } from './views/skins';

import NavStyle from '../../../css/nav.module.css';

export const CenterPanel = (props: { transition?: string }) => {
    const [index, setIndex] = useState(0);

    return (
        <Panel
            style={{ width: '20vw', maxWidth: '500px', minWidth: '320px' }}
            transition={props.transition}
        >
            <Tabs
                selectedIndex={index}
                selectedTabClassName={NavStyle.activeTab}
                onSelect={tabIndex => {
                    setIndex(tabIndex);
                }}
            >
                <TabList className={NavStyle.nav}>
                    <Tab data-tip="Home">
                        <i className="fas fa-home"></i>
                    </Tab>
                    <Tab data-tip="Settings">
                        <i className="fas fa-cog"></i>
                    </Tab>
                    <Tab data-tip="Controls">
                        <i className="fas fa-gamepad"></i>
                    </Tab>
                    <Tab data-tip="Themes">
                        <i className="fas fa-paint-roller"></i>
                    </Tab>
                    <Tab style={{ display: 'none' }}></Tab>
                </TabList>
                <TabPanel>
                    <Home setIndex={setIndex} />
                </TabPanel>
                <TabPanel>
                    <SettingsTab />
                </TabPanel>
                <TabPanel>
                    <Hotkeys />
                </TabPanel>
                <TabPanel>
                    <ThemesTab></ThemesTab>
                </TabPanel>
                <TabPanel>
                    <Skins />
                </TabPanel>
            </Tabs>
        </Panel>
    );
};
