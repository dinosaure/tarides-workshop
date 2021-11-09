open Lwt.Infix

external get_char_from_rx_pin : unit -> char = ""
external set_char_to_tx_pin : char -> unit = ""

let read_byte () : char Lwt.t =
  (* Can we read something? *)
  OS.Main.wait_for_work_on_handle 0 >>= fun () ->
  (* Yes! *)
  let chr = get_char_from_rx_pin () in
  Lwt.return chr

let write_byte chr =
  (* Can we write something? *) 
  OS.Main.wait_for_work_on_handle 1 >>= fun () ->
  (* Yes! *)
  set_char_to_tx_pin chr ;
  Lwt.return_unit
