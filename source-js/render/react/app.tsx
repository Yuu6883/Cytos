import { useEffect } from 'react';

import { CytosUI } from './components/cytos';
import Client from '../client';
export const App = () => {
    useEffect(() => void Client.init());
    return <CytosUI />;
};
