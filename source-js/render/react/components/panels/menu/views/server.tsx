import Client from '../../../../../client';
import Style from '../../../../css/server.module.css';

export const ServerPanel = () => {
    return (
        <div className={Style.server_panel}>
            <button
                onClick={() => Client.instance?.worker.postMessage({ restart: true })}
            >
                Restart
            </button>
        </div>
    );
};
