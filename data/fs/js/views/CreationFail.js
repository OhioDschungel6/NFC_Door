import AbstractView from "./AbstractView.js";

export default class extends AbstractView {
    constructor(params, functions, payload) {
        super(params);
        this.payload = payload;
        this.setTitle("Failure");
    }

    async getHtml() {
        return `
            <div class="chip-creation-failure container">
                <svg class="failure" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 52 52">
                    <circle class="failure__circle" cx="26" cy="26" r="25" fill="none"/>
                    <g>
                      <path class="failure__cross" fill="none" d="M16 16 L36 36"/>
                      <path class="failure__cross" fill="none" d="M16 36 L36 16"/>
                    </g>
                </svg>


                <h2>
                    ${this.payload?.message ?? "Creation failed!"}
                </h2>

                <div class="buttons">
                    <button id="add-card">Try again</button>
                    <button id="to-home">Return to homepage</button>
                </div>
            </div>
        `;
    }

    onCreated(params, functions, payload) {
        const addCard = document.querySelector("#add-card");
        const toHome = document.querySelector("#to-home");

        addCard.addEventListener("click", () => {
            functions.navigateToPath("/new");
        });
        toHome.addEventListener("click", () => {
            functions.navigateToPath("/");
        });
    }
}