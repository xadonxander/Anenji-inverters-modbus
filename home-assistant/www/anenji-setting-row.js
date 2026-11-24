class AnenjiSettingRow extends HTMLElement {
  setConfig(config) {
    if (!config.entity) throw new Error("Entity required");
    this.config = config;
    if (!this.shadowRoot) {
      this.attachShadow({mode:"open"});
      this.shadowRoot.innerHTML = `<style>
      :host{display:block} button{font:inherit;cursor:pointer;color:var(--primary-text-color)}
      .row{display:flex;align-items:center;width:100%;gap:16px;min-height:40px;background:none;border:0;text-align:left;padding:0}
      ha-icon{color:var(--state-icon-color);flex:none} .name{flex:1} .value{color:var(--secondary-text-color)}
      dialog{border:0;border-radius:16px;padding:24px;width:min(360px,85vw);box-sizing:border-box;background:var(--card-background-color);color:var(--primary-text-color);box-shadow:0 8px 32px #0005}
      dialog::backdrop{background:#0007} h2{font-size:20px;margin:0 0 16px} input{box-sizing:border-box;width:100%;padding:12px;font:inherit;color:inherit;background:var(--secondary-background-color);border:1px solid var(--divider-color);border-radius:6px}
      .actions{display:flex;justify-content:flex-end;gap:12px;margin-top:20px}.actions button{border:0;border-radius:8px;padding:10px 16px;background:var(--secondary-background-color)} .actions .save{background:var(--primary-color);color:var(--text-primary-color,#fff)} .error{color:var(--error-color);margin-top:8px}
      </style><button class="row"><ha-icon icon="mdi:tune"></ha-icon><span class="name"></span><span class="value"></span></button><dialog><h2></h2><p class="current"></p><form><input type="number" required><div class="error" role="alert"></div><div class="actions"><button type="button" class="cancel">Cancel</button><button type="submit" class="save">Save</button></div></form></dialog>`;
      const q=s=>this.shadowRoot.querySelector(s);
      q(".row").onclick=()=>this.openEditor();
      q(".cancel").onclick=()=>q("dialog").close();
      q("form").onsubmit=async e=>{
        e.preventDefault();
        const input=q("input"), value=input.valueAsNumber, state=this._hass?.states[this.config.entity];
        if (!state || ["unknown","unavailable"].includes(state.state) || !Number.isFinite(value) || !input.reportValidity()) return;
        q(".save").disabled=true; q(".error").textContent="";
        try {
          await this._hass.callService("number","set_value",{entity_id:this.config.entity,value});
          q("dialog").close();
        } catch(err) { q(".error").textContent=err.message||"Unable to send setting"; }
        finally { q(".save").disabled=false; }
      };
    }
    this.render();
  }
  set hass(hass){this._hass=hass;this.render();}
  render(){
    if(!this.config||!this.shadowRoot)return;
    const s=this._hass?.states[this.config.entity], q=x=>this.shadowRoot.querySelector(x);
    q(".name").textContent=this.config.name||s?.attributes.friendly_name||this.config.entity;
    q(".value").textContent=s ? s.state+(s.attributes.unit_of_measurement?" "+s.attributes.unit_of_measurement:"") : "Unavailable";
    q(".row").disabled=!s||["unknown","unavailable"].includes(s.state);
  }
  openEditor(){
    const s=this._hass?.states[this.config.entity], q=x=>this.shadowRoot.querySelector(x);
    if(!s||["unknown","unavailable"].includes(s.state))return;
    q("h2").textContent=this.config.name||s.attributes.friendly_name;
    q(".current").textContent="Current value: "+s.state+" "+(s.attributes.unit_of_measurement||"");
    const input=q("input");
    input.min=s.attributes.min??"";input.max=s.attributes.max??"";input.step=s.attributes.step??"any";input.value=s.state;
    q(".error").textContent="";q("dialog").showModal();input.focus();input.select();
  }
}
if(!customElements.get("anenji-setting-row"))customElements.define("anenji-setting-row",AnenjiSettingRow);
