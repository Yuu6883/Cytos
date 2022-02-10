// credit to nebula for this file totally didnt take it from him lmao

import { State } from '@hookstate/core';
import { S } from './settings';

interface SettingDetails<T> {
    text?: string;
    min?: number;
    max?: number;
    step?: number;
    values?: Array<[string | number, string]>; // [value, text]
    autoChange?: boolean; // Whether to automatically trigger the change event
    allowDuplicate?: boolean; // For hotkey (allows 1 duplicate)
    dep?: string[];
    minion?: S<T>;
    unit?: string;
    multi?: number;
    precision?: number;
    state?: State<any>;
}

export class Setting<T> {
    public ref: S<T>;
    public value: T;
    public minionValue: T;
    public type: number;
    public valueMap: { [v: string]: string };
    public details: SettingDetails<T>;
    public dep: string[];
    public nowrite: boolean;

    public constructor(
        ref: S<T>,
        type: number,
        details: SettingDetails<T>,
        valueMap: { [v: string]: string } = {},
    ) {
        this.ref = ref;
        this.type = type;
        this.details = details;
        this.valueMap = valueMap;
        this.dep = details.dep || [];
        this.nowrite = false;
    }

    public init() {
        const v = this.parseSetting(
            localStorage.getItem('cytos-settings-' + this.ref.k),
        ) as unknown as T;

        if (v === null || v === undefined) {
            this.v = this.ref.v;
        } else this.v = v;

        if (this.details.min && this.details.max) {
            this.v = Math.min(
                this.details.max,
                Math.max(Number(this.v), this.details.min),
            ) as unknown as T;
        }

        localStorage.setItem('cytos-settings-' + this.ref.k, String(this.v));

        this.details.text = this.valueMap[String(this.v)] || this.details.text;

        if (this.details.minion) {
            this.minionValue = this.parseSetting(
                localStorage.getItem('cytos-settings-minion-' + this.ref.k),
            ) as unknown as T;

            if (this.minionValue === null || this.minionValue === undefined) {
                this.minionValue = this.ref.v;
            }

            this.details.minion.v = this.minionValue;

            localStorage.setItem(
                'cytos-settings-minion-' + this.ref.k,
                String(this.minionValue),
            );
        }

        this.ref.v = this.v;
    }

    public get v() {
        return this.value;
    }

    public set v(value: T) {
        this.value = value;
        this.ref.v = value;

        if (!this.nowrite)
            localStorage.setItem('cytos-settings-' + this.ref.k, String(this.value));

        if (this.details.state) this.details.state.set(value);
    }

    public get getMinionValue() {
        return this.minionValue;
    }

    public set setMinionValue(value: T) {
        this.minionValue = value;
        this.details.minion.v = value;
        if (!this.nowrite)
            localStorage.setItem(
                'cytos-settings-minion-' + this.ref.k,
                String(this.minionValue),
            );
    }

    private parseSetting(value: string) {
        if (typeof this.ref.v === 'string') {
            return value;
        }

        if (typeof this.ref.v === 'boolean') {
            if (value === 'true') return true;
            if (value === 'false') return false;
            return this.ref.v;
        }

        if (typeof this.ref.v === 'number') {
            const number = parseInt(value);
            if (!Number.isNaN(number)) return number;
            return this.ref.v;
        }

        return null;
    }
}
