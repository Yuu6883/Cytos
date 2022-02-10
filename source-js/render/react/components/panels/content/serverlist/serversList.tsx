import { useState as useStore } from '@hookstate/core';

import { ServerData, ServerStore } from '../../../../../stores/servers';

import { ServerItem } from './serverItem';

import Style from '../../../../css/serverlist.module.css';
import { ServerCollapsible } from '../settings/collapsible';

const regions = ['local', 'benchmark'];

const PlaceHolder = { name: '', region: '', players: 0 };

const RegionList = ({ region, servers }: { region: string; servers: ServerData[] }) => {
    const players = 0;

    return (
        <ServerCollapsible
            cat={region}
            players={players}
            disabled={servers.every(s => s.players < 0)}
        >
            {servers.map((server, index) => (
                <ServerItem data={server || PlaceHolder} key={index} />
            ))}
        </ServerCollapsible>
    );
};

export const ServersList = () => {
    const servers = useStore(ServerStore.data).value;

    return (
        <div className={Style.serverlist}>
            {regions.length > 1
                ? regions.map(region => (
                      <RegionList
                          region={region}
                          servers={servers.filter(s => s.region === region)}
                      />
                  ))
                : servers.map((server, index) => (
                      <ServerItem data={server || PlaceHolder} key={index} />
                  ))}
        </div>
    );
};
