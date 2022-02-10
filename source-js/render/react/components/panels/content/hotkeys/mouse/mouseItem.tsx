// credit to nebula for this file totally didnt take it from him lmao

import { useForceUpdate } from '../../../../../utils';

import { MOUSE_ACTIONS } from '../../../../../../client/settings/mouse';
import { Setting } from '../../../../../../client/settings/setting';

import Style from '../../../../../css/hotkeys.module.css';

interface Props {
    hotkey: Setting<number>;
}

function change(setting: Setting<number>, next: boolean, forceUpdate: () => void): void {
    setting.v =
        (MOUSE_ACTIONS.length + setting.value + (next ? 1 : -1)) % MOUSE_ACTIONS.length;
    forceUpdate();
}

export const MouseItem = (props: Props) => {
    const forceUpdate = useForceUpdate();

    return (
        <div className={Style.item}>
            <span>{props.hotkey.details.text}</span>
            <span className={Style.mouse}>
                <i
                    className="fa fa-chevron-left"
                    onClick={() => change(props.hotkey, false, forceUpdate)}
                    aria-hidden="true"
                ></i>
                <b className={Style.active}>{MOUSE_ACTIONS[props.hotkey.ref.v]}</b>
                <i
                    className="fa fa-chevron-right"
                    onClick={() => change(props.hotkey, true, forceUpdate)}
                    aria-hidden="true"
                ></i>
            </span>
        </div>
    );
};
