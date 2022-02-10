import { toast, ToastOptions } from 'react-toastify';

const TOAST_OPTION: (autoClose?: number) => ToastOptions = (autoClose = 3000) => ({
    type: 'dark',
    autoClose,
    pauseOnFocusLoss: false,
    pauseOnHover: false,
});

export const basicPopup = (s: string, timeout = 5000) =>
    s && toast(<h4>{s}</h4>, TOAST_OPTION(timeout));
