import { SettingsList } from '../../content/settings/settingsList';

import Style from '../../../../css/settings.module.css';

export const SettingsTab = (props: { replay?: boolean }) => {
    return (
        <div className={Style.list}>
            <SettingsList isReplay={props.replay} />
        </div>
    );
};
