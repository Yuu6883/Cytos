import { ThemesList } from '../../content/settings/themesList';

import Style from '../../../../css/settings.module.css';

export const ThemesTab = () => {
    return (
        <div className={Style.list}>
            <ThemesList />
        </div>
    );
};
