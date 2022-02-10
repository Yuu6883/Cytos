import { CenterPanel } from './menu/center.panel';
import { RightPanel } from './menu/right.panel';

import Style from '../../css/overlay.module.css';

export const Panels = (props: { showing: boolean }) => (
    <div className={Style.container}>
        <div className={Style.panels}>
            <div
                id="cytos-io_300x600_1"
                className={Style.ads300}
                panel-transition={props.showing ? 'fade-in' : 'fade-out'}
            ></div>
            <div className={Style.panels} style={{ width: '1000px', height: '525px' }}>
                <CenterPanel
                    transition={
                        props.showing
                            ? 'fade-in-slide-in-left'
                            : 'fade-out-slide-out-left'
                    }
                />
                <RightPanel
                    transition={
                        props.showing
                            ? 'fade-in-slide-in-right'
                            : 'fade-out-slide-out-right'
                    }
                />
            </div>
            <div
                id="cytos-io_300x600_2"
                className={Style.ads300}
                panel-transition={props.showing ? 'fade-in' : 'fade-out'}
            ></div>
        </div>
    </div>
);
