import { useEffect } from 'react';
import Style from '../../css/hud.module.css';

export const Select = <V, T extends { label: string; value: V }>(props: {
    style?: React.CSSProperties;
    expand: boolean;
    setExpand: (exp: boolean) => void;
    options: T[];
    value: string;
    onChange: (v: T) => void;
    position: 'center' | 'top';
}) => {
    useEffect(() => {
        if (props.expand) {
            window.addEventListener('click', () => props.setExpand(false), {
                once: true,
            });
        }
    }, [props.expand]);

    const popUpStyle = [Style.selectPopup];
    if (props.position === 'top') popUpStyle.push(Style.top);
    if (props.position === 'center') popUpStyle.push(Style.center);

    return (
        <>
            <div style={props.style}>{props.value}</div>
            {props.expand && (
                <a className={popUpStyle.join(' ')}>
                    {props.options.map((v, i) => (
                        <div
                            key={i}
                            className={Style.selectItem}
                            onClick={e => {
                                e.stopPropagation();
                                props.onChange(v);
                            }}
                        >
                            {v?.label}
                        </div>
                    ))}
                </a>
            )}
        </>
    );
};
