import { useState } from '@hookstate/core';
import { CurrServer } from '../../../../../stores/servers';

export const ServerPanel = () => {
    const usage = useState(CurrServer.usage).value;

    return (
        <div style={{ color: 'white', padding: '10px' }}>
            <h3>Usage: ({(usage * 100).toFixed(1)}%)</h3>
        </div>
    );
};
