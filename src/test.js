const parseDate = (dateString) => {
  let dateParts = dateString.split(/[/, :]/);
  let day = parseInt(dateParts[0]);
  let month = parseInt(dateParts[1]) - 1; // Subtract 1 since months are 0-indexed
  let year = parseInt(dateParts[2]);
  let hour = parseInt(dateParts[4]);
  let minute = parseInt(dateParts[5]);
  let second = parseInt(dateParts[6]);
  
  // Adjust for 12-hour format and PM
  if (dateParts[7].trim().toLowerCase() === "p. m." && hour < 12) {
      hour += 12;
  }
  
  let dateObject = new Date(year, month, day, hour, minute, second);
  return dateObject;
}
