import { useEffect, useRef, useState } from 'react';
import Style from '../../../../css/settings.module.css';
import ServerStyle from '../../../../css/serverlist.module.css';
import ReactTooltip from 'react-tooltip';

interface CollapsibleProps {
    cat: string;
}

export const Collapsible = ({
    cat,
    children,
}: React.PropsWithChildren<CollapsibleProps>) => {
    const [col, setCol] = useState(true);
    const [cd, setCD] = useState(false);
    const containerRef = useRef<HTMLDivElement>();

    useEffect(() => {
        setCD(true);
        const t = setTimeout(() => setCD(false), 250);
        return () => clearTimeout(t);
    }, [col]);

    return (
        <>
            <div className={Style.colHeader} key={cat} onClick={() => cd || setCol(!col)}>
                <span>{cat}</span>
                <i
                    className="fas fa-chevron-left"
                    style={{
                        transform: col ? 'rotate(0deg)' : 'rotate(-90deg)',
                        color: col ? '' : '#ac2a3d',
                    }}
                ></i>
            </div>
            <div
                ref={containerRef}
                className={Style.colContainer}
                style={{
                    maxHeight: col
                        ? '0px'
                        : `${containerRef.current?.scrollHeight || 0}px`,
                }}
            >
                {children}
            </div>
        </>
    );
};

export const ServerCollapsible = ({
    cat,
    players,
    disabled,
    children,
}: React.PropsWithChildren<
    CollapsibleProps & { players: number; disabled: boolean }
>) => {
    const [col, setCol] = useState(false);
    const [tips, setTips] = useState<{ 'data-tip'?: string }>({});
    const [cd, setCD] = useState(false);

    const containerRef = useRef<HTMLDivElement>();

    const toggle = () => disabled || cd || setCol(!col);

    useEffect(() => {
        setCD(true);
        const t = setTimeout(() => setCD(false), 250);
        return () => clearTimeout(t);
    }, [col]);

    useEffect(() => {
        setTips(disabled ? { 'data-tip': 'Region Offline' } : {});
        if (disabled) setCol(true);
        ReactTooltip.rebuild();
    }, [disabled]);

    return (
        <>
            <div
                className={Style.colHeader}
                key={cat}
                onClick={toggle}
                style={{ cursor: disabled ? 'not-allowed' : 'pointer' }}
                {...tips}
            >
                <span>{cat}</span>
                <span className={ServerStyle.playerCount}>
                    {disabled ? (
                        <i className="fas fa-exclamation-triangle"></i>
                    ) : (
                        <>
                            {players} <i className="far fa-user"></i>
                        </>
                    )}
                </span>
                <i
                    className="fas fa-chevron-left"
                    style={{
                        marginLeft: '10px',
                        transform: col ? 'rotate(0deg)' : 'rotate(-90deg)',
                        color: col ? '' : '#ac2a3d',
                    }}
                ></i>
            </div>
            <div
                ref={containerRef}
                className={Style.colContainer}
                style={{
                    maxHeight: col
                        ? '0px'
                        : `${containerRef.current?.scrollHeight || 0}px`,
                }}
            >
                {children}
            </div>
        </>
    );
};
