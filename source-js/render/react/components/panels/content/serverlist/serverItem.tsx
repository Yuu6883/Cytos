import { useEffect, useState } from 'react';
import ReactTooltip from 'react-tooltip';
import Client from '../../../../../client';
import { SYS } from '../../../../../client/util/sys_message';
import { CurrServer, ServerData } from '../../../../../stores/servers';

import Style from '../../../../css/serverlist.module.css';

interface Props {
    data: ServerData;
}

export const ServerItem = ({ data }: Props) => {
    const [tips, setTips] = useState<{ 'data-tip'?: string }>({});

    useEffect(() => {
        setTips(data.players < 0 ? { 'data-tip': 'Server Offline' } : {});
    }, [data.players]);

    useEffect(() => void ReactTooltip.rebuild(), [tips]);

    return (
        <div
            className={`${Style.item} ${data.players < 0 && Style.offline}`}
            onClick={() => {
                if (data.players >= 0) {
                    const mode =
                        data.region === 'benchmark' ? `bench-${data.name}` : data.name;
                    Client.instance.connect(mode);
                    CurrServer.connected.set(true);

                    const name = `${data.region} ${data.name.toLocaleUpperCase()}`;
                    CurrServer.name.set(name);
                    CurrServer.mode.set(mode);
                    SYS.gameConnected(name);
                    ReactTooltip.hide();
                }
            }}
            {...tips}
        >
            <span>{data.name}</span>
            <span className={Style.playerCount}>
                {data.players >= 0 ? (
                    <>
                        {data.players} <i className="far fa-user"></i>
                    </>
                ) : (
                    <i className="fas fa-exclamation-triangle"></i>
                )}
            </span>
        </div>
    );
};
