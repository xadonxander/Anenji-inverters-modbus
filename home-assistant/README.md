# Home Assistant dashboard

Create a new dashboard under Settings > Dashboards, open it, enter edit mode and use the raw configuration editor to paste dashboard.yaml. This uses native cards and the entity IDs from the garage ANENJI manager. Slots with unknown or unavailable serials are hidden.

Optional template-sensors.yaml contains a top-level template section. Merge into configuration.yaml or include as a package; do not duplicate an existing template key. Check configuration before restarting. Add the resulting sensors to the overview if desired. Battery net power is estimated from each inverter's signed battery current times voltage, positive discharge. It is not an independent BMS measurement. Grid line power is not verified utility-meter import/export. Settings writes and register browsing are not implemented yet.
